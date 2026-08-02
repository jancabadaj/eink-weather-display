import { $ } from '../app.js';

const PANEL_W = 800;
const PANEL_H = 480;
const GRID_MIN_SCALE = 3; // below this, 1px cells collapse into a wash

let zoom = 'fit';
let currentVersion = -1;

function layout() {
    const stage = $('stage');
    const wrap = $('frame-wrap');

    if (zoom === 'fit') {
        const pad = 10; // room for the border + shadow
        const s = Math.min((wrap.clientWidth - pad) / PANEL_W, (wrap.clientHeight - pad) / PANEL_H);
        stage.style.width = Math.max(1, Math.floor(PANEL_W * s)) + 'px';
        stage.style.height = Math.max(1, Math.floor(PANEL_H * s)) + 'px';
    } else {
        stage.style.width = PANEL_W * zoom + 'px';
        stage.style.height = PANEL_H * zoom + 'px';
    }

    const scale = stage.clientWidth / PANEL_W;

    const overlay = $('grid-overlay');
    overlay.style.setProperty('--cell', scale + 'px');
    overlay.style.setProperty('--major', scale * 10 + 'px');

    const gridUsable = scale >= GRID_MIN_SCALE;
    $('grid').disabled = !gridUsable;
    $('grid-label').classList.toggle('disabled', !gridUsable);
    overlay.classList.toggle('on', gridUsable && $('grid').checked);

    wrap.classList.toggle(
        'pannable',
        stage.clientWidth > wrap.clientWidth || stage.clientHeight > wrap.clientHeight
    );

    for (const b of document.querySelectorAll('[data-zoom]')) {
        b.setAttribute('aria-pressed', String(b.dataset.zoom === String(zoom)));
    }
}

function init() {
    for (const b of document.querySelectorAll('[data-mode]')) {
        b.onclick = () => fetch('/api/mode/' + b.dataset.mode, { method: 'POST' });
    }

    for (const b of document.querySelectorAll('[data-zoom]')) {
        b.onclick = () => {
            zoom = b.dataset.zoom === 'fit' ? 'fit' : Number(b.dataset.zoom);
            layout();
        };
    }

    $('grid').onchange = layout;
    addEventListener('resize', layout);
    $('frame').addEventListener('load', layout);

    // Display-pixel coordinates under the cursor.
    $('stage').addEventListener('mousemove', (e) => {
        const r = $('frame').getBoundingClientRect();
        const scale = r.width / PANEL_W;
        const x = Math.floor((e.clientX - r.left) / scale);
        const y = Math.floor((e.clientY - r.top) / scale);
        $('cursor').textContent =
            x >= 0 && y >= 0 && x < PANEL_W && y < PANEL_H ? `x ${x}  y ${y}` : '—';
    });
    $('frame-wrap').addEventListener('mouseleave', () => ($('cursor').textContent = '—'));

    // Drag to pan once a zoom level overflows the viewport.
    const wrap = $('frame-wrap');
    let from = null;
    wrap.addEventListener('mousedown', (e) => {
        if (!wrap.classList.contains('pannable')) return;
        from = { x: e.clientX, y: e.clientY, left: wrap.scrollLeft, top: wrap.scrollTop };
        wrap.classList.add('panning');
        e.preventDefault();
    });
    addEventListener('mousemove', (e) => {
        if (!from) return;
        wrap.scrollLeft = from.left - (e.clientX - from.x);
        wrap.scrollTop = from.top - (e.clientY - from.y);
    });
    addEventListener('mouseup', () => {
        from = null;
        wrap.classList.remove('panning');
    });
}

// Always shows the compiler's own output verbatim. On failure it opens with the
// error lines pulled to the top; on success it collapses out of the way.
export function showBuildOutput(build, prefix) {
    const box = $(prefix);
    const output = (build.output || '').trimEnd();
    const errorLines = build.errorLines || [];

    box.style.display = output ? 'block' : 'none';
    box.classList.toggle('failed', !build.ok);
    box.open = !build.ok;

    $(prefix + '-summary').textContent = build.ok
        ? `compiler output (${output.split('\n').length} lines)`
        : `build failed${errorLines.length ? ` \u2014 ${errorLines.length} error(s)` : ''}`;

    $(prefix + '-text').textContent = errorLines.length
        ? errorLines.join('\n') + '\n\n--- full output ---\n' + output
        : output;
}

function apply(s) {
    $('s-status').textContent = s.building ? 'building…' : s.build.ok ? 'ok' : 'failed';
    $('s-status').style.color = s.build.ok ? 'var(--good)' : 'var(--bad)';
    $('s-duration').textContent = s.lastBuildMs ? s.lastBuildMs + ' ms' : '—';
    $('s-frame').textContent = '#' + s.frameVersion;

    showBuildOutput(s.build, 'build-output');

    for (const b of document.querySelectorAll('[data-mode]')) {
        b.setAttribute('aria-pressed', String(b.dataset.mode === s.mode));
    }

    if (s.frameVersion !== currentVersion) {
        currentVersion = s.frameVersion;
        $('frame').src = '/api/frame.bmp?v=' + s.frameVersion;
    }
}

export default {
    id: 'preview',
    title: 'Preview',
    init,
    apply,
    onShow: layout,
    badge: (s) => (s && !s.build.ok ? { text: '!', cls: 'fail' } : { text: '', cls: '' }),
};
