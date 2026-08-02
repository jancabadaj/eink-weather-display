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

// Temporarily excluded - references Arduino / ESP headers
const NOT_YET_HOST_CLEAN = [
    'logger.cpp',
    'provider/auth.cpp',
    'schedule/serverClock.cpp',
    'schedule/updateScheduler.cpp',
    'settings/configOverrides.cpp',
    'weatherCore.cpp',
    'web/webServer.cpp',
];

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
        .filter((f) => !NOT_YET_HOST_CLEAN.includes(f))
        .sort();
}

function run(cmd, args, opts = {}) {
    return new Promise((resolve) => {
        execFile(cmd, args, { maxBuffer: 8 * 1024 * 1024, ...opts }, (err, stdout, stderr) =>
            resolve({ ok: !err, code: err ? err.code : 0, stdout, stderr })
        );
    });
}

const DIAG_START = /^(\S.*?):(\d+):(\d+):\s+(error|warning|note):/;

// Splits compiler output into diagnostics so a real error is never buried under
// unrelated warnings. Paths are made repo-relative for readability.
function parseDiagnostics(output) {
    const errors = [];
    const warnings = [];
    let current = null;

    const flush = () => {
        if (!current) return;
        const text = current.lines.join('\n');
        (current.kind === 'error' ? errors : warnings).push(text);
        current = null;
    };

    for (const raw of (output || '').split('\n')) {
        const line = raw.replace(new RegExp(ROOT + '/', 'g'), '');
        const m = line.match(DIAG_START);
        if (m) {
            // `note:` continues whatever diagnostic it belongs to
            if (m[4] === 'note' && current) {
                current.lines.push(line);
                continue;
            }
            flush();
            current = { kind: m[4], lines: [line] };
        } else if (current) {
            if (/^\d+ (warning|error)s? generated\.$/.test(line.trim())) flush();
            else current.lines.push(line);
        }
    }
    flush();
    return { errors, warnings };
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
        path.join(HARNESS, 'preview_main.cpp'),
        ...sourcesToCompile().map((f) => path.join(SRC, f)),
        '-o',
        binary,
    ];

    const res = await run(CXX, args, { cwd: ROOT });
    const diags = parseDiagnostics(res.stderr || res.stdout);

    return {
        ok: res.ok,
        errors: diags.errors,
        warnings: diags.warnings,
        binary,
    };
}

// Renders one screen. Leaves the previous frame in place on failure so the UI
// keeps showing the last good image.
async function render(mode) {
    const binary = path.join(DIST, 'preview');
    if (!fs.existsSync(binary)) return { ok: false, error: 'preview binary not built' };

    const out = path.join(DIST, 'frame.bmp');
    const tmp = out + '.tmp';
    const res = await run(binary, [mode, tmp], { cwd: DIST });
    if (!res.ok) {
        return { ok: false, error: res.stderr || `preview exited with ${res.code}` };
    }
    fs.renameSync(tmp, out);
    return { ok: true, error: '' };
}

module.exports = { buildPreview, render, DIST, SRC, HARNESS };
