'use strict';

const express = require('express');
const chokidar = require('chokidar');
const { WebSocketServer } = require('ws');
const fs = require('fs');
const path = require('path');

const { buildPreview, render, DIST, SRC, HARNESS } = require('./build');

const PORT = process.env.PORT || 3000;
const app = express();

const state = {
    mode: 'weather',
    frameVersion: 0,
    build: { ok: false, errors: [], warnings: [] },
    building: false,
    lastBuildMs: 0,
};

let sockets = new Set();

function broadcast() {
    const msg = JSON.stringify({ type: 'state', state: publicState() });
    for (const ws of sockets) {
        if (ws.readyState === 1) ws.send(msg);
    }
}

function publicState() {
    const { mode, frameVersion, build, building, lastBuildMs } = state;
    return { mode, frameVersion, build, building, lastBuildMs };
}

async function rebuild(reason) {
    if (state.building) return;
    state.building = true;
    broadcast();

    const started = Date.now();
    const result = await buildPreview();
    state.build = { ok: result.ok, errors: result.errors, warnings: result.warnings };

    if (result.ok) {
        const r = await render(state.mode);
        if (r.ok) state.frameVersion++;
        else state.build = { ...state.build, ok: false, errors: [r.error] };
    }

    state.lastBuildMs = Date.now() - started;
    state.building = false;

    const status = state.build.ok ? 'ok' : 'FAILED';
    console.log(`[build] ${reason} -> ${status} (${state.lastBuildMs}ms)`);
    if (!state.build.ok) console.log(state.build.errors.join('\n'));

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
        else state.build = { ...state.build, ok: false, errors: [r.error] };
    }
    broadcast();
    res.json(publicState());
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
    .watch([SRC, HARNESS], { ignoreInitial: true })
    .on('all', (_event, file) => scheduleRebuild(path.basename(file)));

rebuild('startup');
