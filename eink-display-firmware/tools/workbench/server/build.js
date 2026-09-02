'use strict';

const { execFile } = require('child_process');
const fs = require('fs');
const path = require('path');

const ROOT = path.resolve(__dirname, '../../..'); // eink-display-firmware/
const SRC = path.join(ROOT, 'src');
const HARNESS = path.resolve(__dirname, '../harness');
const DIST = path.resolve(__dirname, '../dist');

const CXX = process.env.CXX || 'c++';
const STD = '-std=gnu++17';

// Arduino by design, never part of a host build.
const EXCLUDED = ['main.cpp', 'platform/arduino'];

function walk(dir, out = []) {
    for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
        const full = path.join(dir, entry.name);
        if (entry.isDirectory()) walk(full, out);
        else if (entry.name.endsWith('.cpp')) out.push(full);
    }
    return out;
}

function sourcesToCompile() {
    const skip = [...EXCLUDED];
    return walk(SRC)
        .map((f) => path.relative(SRC, f).split(path.sep).join('/'))
        .filter((f) => !skip.some((s) => f === s || f.startsWith(s + '/')))
        .sort();
}

function run(cmd, args, opts = {}) {
    return new Promise((resolve) => {
        execFile(cmd, args, { maxBuffer: 8 * 1024 * 1024, ...opts }, (err, stdout, stderr) => {
            const output = [stderr, stdout]
                .filter(Boolean)
                .join('')
                .split(ROOT + '/')
                .join('');
            resolve({
                ok: !err,
                code: err ? (err.code ?? 1) : 0,
                stdout,
                output,
                // Nothing is filtered out, but error lines are worth pulling to the top.
                errorLines: output.split('\n').filter((l) => /(^|\s)error:/.test(l)),
            });
        });
    });
}

// Compiles the preview harness against every host-clean application source.
async function buildPreview() {
    fs.mkdirSync(DIST, { recursive: true });

    const binary = path.join(DIST, 'preview');

    const args = [
        STD,
        '-Wall',
        '-Wextra',
        `-I${SRC}`,
        `-I${HARNESS}`,
        `-I${ARDUINOJSON_INC}`,
        path.join(HARNESS, 'preview_main.cpp'),
        ...sourcesToCompile().map((f) => path.join(SRC, f)),
        '-o',
        binary,
    ];

    const res = await run(CXX, args, { cwd: ROOT });
    return { ok: res.ok, output: res.output, errorLines: res.errorLines, binary };
}

// Renders one screen. Leaves the previous frame in place on failure so the UI
// keeps showing the last good image.
// Renders run one at a time. Two overlapping ones would otherwise write the
// same temp file and the slower one would find it already renamed away.
let renderQueue = Promise.resolve();
let renderSequence = 0;

function render(mode) {
    const next = renderQueue.then(() => renderOnce(mode));
    // Keep the chain alive even when one render rejects.
    renderQueue = next.catch(() => {});
    return next;
}

async function renderOnce(mode) {
    const binary = path.join(DIST, 'preview');
    if (!fs.existsSync(binary)) return { ok: false, error: 'preview binary not built' };

    const out = path.join(DIST, 'frame.bmp');
    const tmp = `${out}.${process.pid}.${++renderSequence}.tmp`;
    const res = await run(binary, [mode, tmp], { cwd: DIST });
    if (!res.ok) {
        fs.rmSync(tmp, { force: true });
        return { ok: false, error: res.output || `preview exited with ${res.code}` };
    }

    try {
        fs.renameSync(tmp, out);
    } catch (err) {
        return { ok: false, error: `preview wrote no frame: ${err.message}` };
    }
    return { ok: true, error: '' };
}

// Renders the device's admin page from a supplied status snapshot.
async function buildAdmin() {
    fs.mkdirSync(DIST, { recursive: true });

    const binary = path.join(DIST, 'admin');
    const args = [
        STD,
        '-Wall',
        '-Wextra',
        `-I${SRC}`,
        `-I${HARNESS}`,
        `-I${ARDUINOJSON_INC}`,
        path.join(HARNESS, 'admin_main.cpp'),
        ...sourcesToCompile().map((f) => path.join(SRC, f)),
        '-o',
        binary,
    ];

    const res = await run(CXX, args, { cwd: ROOT });
    return { ok: res.ok, output: res.output, errorLines: res.errorLines, binary };
}

async function renderAdmin(snapshot) {
    const binary = path.join(DIST, 'admin');
    if (!fs.existsSync(binary)) return { ok: false, error: 'admin binary not built' };

    const statePath = path.join(DIST, 'admin-state.json');
    const out = path.join(DIST, 'admin.html');
    fs.writeFileSync(statePath, JSON.stringify(snapshot ?? {}));

    const res = await run(binary, [statePath, out], { cwd: DIST });
    if (!res.ok) {
        return { ok: false, error: res.output || 'admin renderer failed' };
    }
    return { ok: true, error: '' };
}

const TEST_DIR = path.join(ROOT, 'test');

// doctest is declared in platformio.ini's `native` env and fetched with
// `pio pkg install -e native`, so the version is managed in one place and can
// never reach the firmware build.
const DOCTEST_INC = path.join(ROOT, '.pio', 'libdeps', 'native', 'doctest', 'doctest');

// ArduinoJson is platform-independent and already fetched for the firmware, so
// the host build compiles the parsers against the exact same version.
const ARDUINOJSON_INC = path.join(ROOT, '.pio', 'libdeps', 'esp32dev', 'ArduinoJson', 'src');

function testSources() {
    return fs
        .readdirSync(TEST_DIR)
        .filter((f) => f.endsWith('.cpp'))
        .sort()
        .map((f) => path.join(TEST_DIR, f));
}

// Catches a TEST_CASE that is declared but never runs - a duplicate name, a case
// excluded by a filter, or a file dropped from the build.
//
// It does NOT catch a deleted test: if the declaration goes, this count drops
// with it and the comparison still matches. Only review or git catches that.
function declaredTestCount() {
    return testSources()
        .map((f) => fs.readFileSync(f, 'utf8'))
        .join('\n')
        .split('\n')
        .filter((l) => /^\s*TEST_CASE(_TEMPLATE)?\s*\(/.test(l)).length;
}

function doctestMissing() {
    return !fs.existsSync(path.join(DOCTEST_INC, 'doctest.h'));
}

// Compiles every *_test.cpp against the same host sources the preview uses.
async function buildTests() {
    fs.mkdirSync(DIST, { recursive: true });

    if (doctestMissing()) {
        return {
            ok: false,
            output:
                'doctest not installed.\n\nRun:  pio pkg install -e native\n\n' +
                `Expected header at ${path.relative(ROOT, DOCTEST_INC)}/doctest.h`,
            errorLines: ['doctest not installed - run: pio pkg install -e native'],
        };
    }

    const binary = path.join(DIST, 'tests');
    const args = [
        STD,
        '-Wall',
        '-Wextra',
        `-I${SRC}`,
        `-I${DOCTEST_INC}`,
        `-I${ARDUINOJSON_INC}`,
        `-DPROJECT_ROOT="${ROOT}"`,
        ...testSources(),
        ...sourcesToCompile().map((f) => path.join(SRC, f)),
        '-o',
        binary,
    ];

    const res = await run(CXX, args, { cwd: ROOT });
    return { ok: res.ok, output: res.output, errorLines: res.errorLines, binary };
}

// --- doctest XML -----------------------------------------------------------
// Without --success, doctest still emits every TestCase but only expands the
// failing assertions, which is exactly what the panel needs.

function attrs(s) {
    const out = {};
    for (const m of s.matchAll(/(\w+)="([^"]*)"/g)) out[m[1]] = m[2];
    return out;
}

function unescapeXml(s) {
    return (s || '')
        .trim()
        .replace(/&lt;/g, '<')
        .replace(/&gt;/g, '>')
        .replace(/&quot;/g, '"')
        .replace(/&apos;/g, "'")
        .replace(/&amp;/g, '&');
}

function tagText(body, tag) {
    const m = new RegExp(`<${tag}>([\\s\\S]*?)</${tag}>`).exec(body);
    return m ? unescapeXml(m[1]) : '';
}

function parseDoctestXml(xml) {
    const tests = [];

    for (const c of xml.matchAll(/<TestCase\b([^>]*?)(?:\/>|>([\s\S]*?)<\/TestCase>)/g)) {
        const a = attrs(c[1]);
        const body = c[2] || '';
        const overall = attrs((/<OverallResultsAsserts([^>]*)\/>/.exec(body) || [, ''])[1]);

        const failures = [];

        for (const e of body.matchAll(/<Expression\b([^>]*)>([\s\S]*?)<\/Expression>/g)) {
            const ea = attrs(e[1]);
            if (ea.success === 'true') continue;
            failures.push({
                kind: ea.type || 'CHECK',
                line: Number(ea.line || 0),
                file: path.relative(ROOT, ea.filename || ''),
                original: tagText(e[2], 'Original'),
                expanded: tagText(e[2], 'Expanded'),
            });
        }

        // FAIL(...) and friends arrive as Message rather than Expression.
        for (const m of body.matchAll(/<Message\b([^>]*)>([\s\S]*?)<\/Message>/g)) {
            const ma = attrs(m[1]);
            failures.push({
                kind: ma.type || 'FAIL',
                line: Number(ma.line || 0),
                file: path.relative(ROOT, ma.filename || ''),
                original: tagText(m[2], 'Text'),
                expanded: '',
            });
        }

        const [suite, ...rest] = (a.name || '').split(': ');
        tests.push({
            suite: rest.length ? suite : '',
            name: rest.length ? rest.join(': ') : a.name || '',
            ok: overall.test_case_success === 'true' && failures.length === 0,
            asserts: { passed: Number(overall.successes || 0), failed: Number(overall.failures || 0) },
            failures,
        });
    }

    const totals = attrs((/<OverallResultsTestCases([^>]*)\/>/.exec(xml) || [, ''])[1]);
    return {
        tests,
        passed: Number(totals.successes || 0),
        failed: Number(totals.failures || 0),
    };
}

// Runs the suite. doctest exits with the failure count, so a non-zero exit is a
// normal result rather than a tooling error.
async function runTests() {
    const binary = path.join(DIST, 'tests');
    if (!fs.existsSync(binary)) return { ok: false, error: 'test binary not built' };

    const started = Date.now();
    const res = await run(binary, ['--reporters=xml'], { cwd: ROOT });
    const xml = res.stdout || '';

    if (!xml.includes('<doctest')) {
        return { ok: false, error: res.output || 'test runner produced no output' };
    }

    const results = parseDoctestXml(xml);
    results.durationMs = Date.now() - started;

    const declared = declaredTestCount();
    if (results.tests.length !== declared) {
        return {
            ok: false,
            error:
                `Test count mismatch: ${declared} TEST_CASE(s) declared in test/, ` +
                `but doctest ran ${results.tests.length}.\n` +
                'A test was lost, or one failed to register.',
        };
    }

    return { ok: true, results };
}

module.exports = {
    buildPreview,
    render,
    buildAdmin,
    renderAdmin,
    buildTests,
    runTests,
    declaredTestCount,
    DIST,
    SRC,
    HARNESS,
    TEST_DIR,
};
