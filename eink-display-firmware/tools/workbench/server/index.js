'use strict';

const express = require('express');
const chokidar = require('chokidar');
const { WebSocketServer } = require('ws');
const fs = require('fs');
const path = require('path');

const { buildPreview, render, buildAdmin, renderAdmin, buildTests, runTests, DIST, SRC, HARNESS, TEST_DIR } = require('./build');

const PORT = process.env.PORT || 3000;
const app = express();

const state = {
    mode: 'weather',
    frameVersion: 0,
    build: { ok: false, output: '', errorLines: [] },
    building: false,
    lastBuildMs: 0,
    tests: {
        ok: false,
        output: '',
        errorLines: [],
        results: null,
        lastRunMs: 0,
        running: false,
        autoRun: true, // "Rerun on save" checkbox
    },
    testsPanelOpen: false, // set by the client when the Tests tab is shown

    admin: {
        ok: false,
        output: '',
        errorLines: [],
        version: 0,
        snapshot: {
            loggedIn: true,
            updatesStopped: false,
            nightStartHour: 21,
            nightEndHour: 5,
            nightOverridden: false,
            localAddress: '192.168.1.50',
        },
    },
    adminPanelOpen: false,
};

let sockets = new Set();

function broadcast() {
    const msg = JSON.stringify({ type: 'state', state: publicState() });
    for (const ws of sockets) {
        if (ws.readyState === 1) ws.send(msg);
    }
}

function publicState() {
    const { mode, frameVersion, build, building, lastBuildMs, tests, admin } = state;
    return { mode, frameVersion, build, building, lastBuildMs, tests, admin };
}

async function rebuild(reason) {
    if (state.building) return;
    state.building = true;
    broadcast();

    try {
        await rebuildInner(reason);
    } catch (err) {
        // A tool-side failure must never take the server down: the browser tab
        // is meant to survive anything the build does.
        console.error('[build] unexpected failure:', err);
        state.build = { ok: false, output: String(err && err.stack ? err.stack : err), errorLines: [] };
    } finally {
        state.building = false;
        broadcast();
    }
}

async function rebuildInner(reason) {

    const started = Date.now();
    const result = await buildPreview();
    state.build = { ok: result.ok, output: result.output, errorLines: result.errorLines };

    if (result.ok) {
        const r = await render(state.mode);
        if (r.ok) state.frameVersion++;
        else state.build = { ...state.build, ok: false, output: r.error };
    }

    state.lastBuildMs = Date.now() - started;

    const status = state.build.ok ? 'ok' : 'FAILED';
    console.log(`[build] ${reason} -> ${status} (${state.lastBuildMs}ms)`);
    if (state.build.output.trim()) console.log(state.build.output.trimEnd());

    broadcast();

    // Only pay for these builds while someone is actually watching them.
    if (state.adminPanelOpen) {
        await rerenderAdmin(reason);
    }
    if (state.testsPanelOpen && state.tests.autoRun) {
        await rerunTests(reason);
    }
}

async function rerenderAdmin(reason) {
    const built = await buildAdmin();
    if (!built.ok) {
        state.admin = { ...state.admin, ok: false, output: built.output, errorLines: built.errorLines };
        console.log(`[admin] ${reason} -> BUILD FAILED`);
        broadcast();
        return;
    }

    const rendered = await renderAdmin(state.admin.snapshot);
    state.admin = {
        ...state.admin,
        ok: rendered.ok,
        output: built.output,
        errorLines: rendered.ok ? [] : [rendered.error],
        version: state.admin.version + 1,
    };
    console.log(`[admin] ${reason} -> ${rendered.ok ? 'rendered' : 'FAILED'}`);
    broadcast();
}

async function rerunTests(reason) {
    if (state.tests.running) return;
    state.tests = { ...state.tests, running: true };
    broadcast();

    const started = Date.now();
    const built = await buildTests();

    if (!built.ok) {
        state.tests = {
            ...state.tests,
            ok: false,
            output: built.output,
            errorLines: built.errorLines,
            results: null,
            lastRunMs: Date.now() - started,
            running: false,
        };
        console.log(`[tests] ${reason} -> BUILD FAILED`);
        broadcast();
        return;
    }

    const run = await runTests();
    state.tests = {
        ...state.tests,
        ok: run.ok && run.results.failed === 0,
        output: run.ok ? built.output : built.output + '\n' + run.error,
        errorLines: run.ok ? [] : [run.error],
        results: run.ok ? run.results : null,
        lastRunMs: Date.now() - started,
        running: false,
    };

    if (run.ok) {
        const { passed, failed } = run.results;
        console.log(`[tests] ${reason} -> ${failed ? failed + ' FAILED' : 'all passed'} (${passed + failed} tests, ${state.tests.lastRunMs}ms)`);
    } else {
        console.log(`[tests] ${reason} -> runner error: ${run.error}`);
    }

    broadcast();
}

app.use(express.static(path.resolve(__dirname, '../web')));

app.get('/api/state', (_req, res) => res.json(publicState()));

app.get('/api/frame.bmp', (_req, res) => {
    const file = path.join(DIST, 'frame.bmp');
    if (!fs.existsSync(file)) return res.status(404).end();
    res.set('Cache-Control', 'no-store');
    res.type('image/bmp').send(fs.readFileSync(file));
});

app.post('/api/mode/:mode', async (req, res) => {
    const allowed = ['weather', 'night-mode', 'network-error'];
    if (!allowed.includes(req.params.mode)) return res.status(400).json({ error: 'unknown mode' });

    state.mode = req.params.mode;
    if (state.build.ok) {
        const r = await render(state.mode);
        if (r.ok) state.frameVersion++;
        else state.build = { ...state.build, ok: false, output: r.error };
    }
    broadcast();
    res.json(publicState());
});

app.use(express.json());

app.get('/api/admin.html', (_req, res) => {
    const file = path.join(DIST, 'admin.html');
    if (!fs.existsSync(file)) return res.status(404).end();
    res.set('Cache-Control', 'no-store');
    res.type('text/html').send(fs.readFileSync(file));
});

app.post('/api/admin/active', async (req, res) => {
    const open = !!req.body.active;
    const wasClosed = !state.adminPanelOpen;
    state.adminPanelOpen = open;
    res.json({ ok: true });

    if (open && wasClosed && state.admin.version === 0) {
        await rerenderAdmin('admin panel opened');
    }
});

app.post('/api/admin/snapshot', async (req, res) => {
    state.admin = { ...state.admin, snapshot: { ...state.admin.snapshot, ...req.body } };
    res.json({ ok: true });
    await rerenderAdmin('snapshot changed');
});

app.post('/api/tests/active', async (req, res) => {
    const open = !!req.body.active;
    const wasClosed = !state.testsPanelOpen;
    state.testsPanelOpen = open;
    res.json({ ok: true });

    // First look at the panel: run once so it is never blank.
    if (open && wasClosed && !state.tests.results && !state.tests.running) {
        await rerunTests('tests panel opened');
    }
});

app.post('/api/tests/autorun', (req, res) => {
    state.tests = { ...state.tests, autoRun: !!req.body.enabled };
    res.json({ ok: true });
    broadcast();
});

app.post('/api/tests/run', async (req, res) => {
    res.json({ ok: true });
    await rerunTests('manual run');
});

const server = app.listen(PORT, () => {
    console.log(`workbench -> http://localhost:${PORT}`);
});

const wss = new WebSocketServer({ server });
wss.on('connection', (ws) => {
    sockets.add(ws);
    ws.send(JSON.stringify({ type: 'state', state: publicState() }));
    ws.on('close', () => sockets.delete(ws));
});

let pending = null;
function scheduleRebuild(reason) {
    clearTimeout(pending);
    pending = setTimeout(() => rebuild(reason), 80); // debounce editor multi-writes
}

chokidar
    .watch([SRC, HARNESS, TEST_DIR], { ignoreInitial: true })
    .on('all', (_event, file) => scheduleRebuild(path.basename(file)));

rebuild('startup');
