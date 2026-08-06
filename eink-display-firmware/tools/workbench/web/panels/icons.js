import { $, esc } from '../app.js';

// The panel's own root; panels share a document, so queries stay inside it.
let root = document;

// Bitmaps are packed MSB-first with each row padded to whole bytes, matching
// how DrawUtils::drawShape walks them.

let width = 24;
let height = 24;
let pixels = new Uint8Array(width * height);

let zoom = 0; // 0 means fit to the available space
let tool = 'pen';
let sourceImage = null; // kept so threshold and dither can be re-applied

function bytesPerRow() {
    return Math.ceil(width / 8);
}

function status(text, cls = '') {
    $('i-status').textContent = text;
    $('i-status').className = cls;
}

function codeStatus(text, bad = false) {
    $('i-code-status').textContent = text;
    $('i-code-status').style.color = bad ? 'var(--bad)' : 'var(--good)';
}

// --- code -------------------------------------------------------------------

function generateCode() {
    const name = $('i-name').value.trim() || 'NewIcon';

    const lines = [];
    for (let y = 0; y < height; y++) {
        const row = new Uint8Array(bytesPerRow());
        let art = '';
        for (let x = 0; x < width; x++) {
            if (pixels[y * width + x]) row[x >> 3] |= 0x80 >> (x & 7);
            art += pixels[y * width + x] ? '#' : ' ';
        }
        const bytes = Array.from(row)
            .map((b) => '0x' + b.toString(16).toUpperCase().padStart(2, '0'))
            .join(', ');
        lines.push(`    ${bytes},  //${art}`);
    }

    return (
        `#pragma once\n\n#include "../shape.h"\n\n` +
        `// ${name} bitmap (${width}x${height})\n` +
        `const uint8_t ${name}_Bitmap[] = {\n${lines.join('\n')}\n};\n\n` +
        `Shape ${name} = {\n    .bitmap = ${name}_Bitmap,\n` +
        `    .width = ${width},\n    .height = ${height}\n};\n`
    );
}

// Recovers a drawing from a header this panel produced.
function parseHeader(text) {
    const dims = /bitmap \((\d+)x(\d+)\)/.exec(text);
    const body = /_Bitmap\[\]\s*=\s*\{([\s\S]*?)\};/.exec(text);
    if (!dims || !body) return null;

    const w = Number(dims[1]);
    const h = Number(dims[2]);
    const bytes = [...body[1].matchAll(/0x([0-9A-Fa-f]{2})/g)].map((m) => parseInt(m[1], 16));

    const perRow = Math.ceil(w / 8);
    if (bytes.length < perRow * h) return null;

    const out = new Uint8Array(w * h);
    for (let y = 0; y < h; y++) {
        for (let x = 0; x < w; x++) {
            out[y * w + x] = bytes[y * perRow + (x >> 3)] & (0x80 >> (x & 7)) ? 1 : 0;
        }
    }
    return { width: w, height: h, pixels: out };
}

// --- canvas -----------------------------------------------------------------

function currentScale() {
    if (zoom > 0) return zoom;

    // Collapse the canvas first: while it is still large the wrapper carries
    // scrollbars, and measuring around them under-reports the space available.
    const canvas = $('i-canvas');
    const wrap = canvas.parentElement;
    canvas.width = 0;
    canvas.height = 0;

    const availableW = wrap.clientWidth - 24;
    const availableH = wrap.clientHeight - 24;

    // Each axis constrains independently; the tighter one wins.
    return Math.max(1, Math.floor(Math.min(availableW / width, availableH / height)));
}

function draw() {
    const scale = currentScale();
    const canvas = $('i-canvas');
    canvas.width = width * scale;
    canvas.height = height * scale;

    const ctx = canvas.getContext('2d');
    ctx.fillStyle = '#fff';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    ctx.fillStyle = '#000';
    for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x++) {
            if (pixels[y * width + x]) ctx.fillRect(x * scale, y * scale, scale, scale);
        }
    }

    // A grid only helps once cells are big enough to aim at.
    if (scale >= 6) {
        ctx.strokeStyle = 'rgba(120,140,180,.35)';
        ctx.lineWidth = 1;
        for (let x = 0; x <= width; x++) {
            ctx.beginPath();
            ctx.moveTo(x * scale + 0.5, 0);
            ctx.lineTo(x * scale + 0.5, canvas.height);
            ctx.stroke();
        }
        for (let y = 0; y <= height; y++) {
            ctx.beginPath();
            ctx.moveTo(0, y * scale + 0.5);
            ctx.lineTo(canvas.width, y * scale + 0.5);
            ctx.stroke();
        }
    }

    $('i-zoom-label').textContent = scale + '×';
    $('i-code').textContent = generateCode();
}

function pixelAt(event) {
    const rect = $('i-canvas').getBoundingClientRect();
    const scale = currentScale();
    return {
        x: Math.floor((event.clientX - rect.left) / scale),
        y: Math.floor((event.clientY - rect.top) / scale),
    };
}

function paint(event, value) {
    const { x, y } = pixelAt(event);
    const radius = tool === 'brush' ? Math.max(1, Number($('i-radius').value)) : 1;

    if (radius === 1) {
        if (x >= 0 && y >= 0 && x < width && y < height) pixels[y * width + x] = value;
    } else {
        const r = radius - 1;
        for (let dy = -r; dy <= r; dy++) {
            for (let dx = -r; dx <= r; dx++) {
                if (dx * dx + dy * dy > r * r) continue; // round brush
                const px = x + dx;
                const py = y + dy;
                if (px >= 0 && py >= 0 && px < width && py < height) pixels[py * width + px] = value;
            }
        }
    }
    draw();
}

function resize(w, h) {
    const next = new Uint8Array(w * h);
    for (let y = 0; y < Math.min(h, height); y++) {
        for (let x = 0; x < Math.min(w, width); x++) next[y * w + x] = pixels[y * width + x];
    }
    width = w;
    height = h;
    pixels = next;
    $('i-width').value = width;
    $('i-height').value = height;
}

// --- image import -----------------------------------------------------------

// Grey levels from the image, scaled into the current canvas size.
function sampleImage(image, w, h) {
    const canvas = document.createElement('canvas');
    canvas.width = w;
    canvas.height = h;

    const ctx = canvas.getContext('2d', { willReadFrequently: true });
    ctx.fillStyle = '#fff';
    ctx.fillRect(0, 0, w, h);
    ctx.drawImage(image, 0, 0, w, h);

    const data = ctx.getImageData(0, 0, w, h).data;
    const grey = new Float32Array(w * h);
    for (let i = 0; i < w * h; i++) {
        const alpha = data[i * 4 + 3] / 255;
        const lum = 0.299 * data[i * 4] + 0.587 * data[i * 4 + 1] + 0.114 * data[i * 4 + 2];
        grey[i] = lum * alpha + 255 * (1 - alpha); // transparent reads as paper
    }
    return grey;
}

// Scales the image into whatever canvas size is currently set; the canvas is
// only resized when explicitly asked for.
function applyImage() {
    if (!sourceImage) return;

    const threshold = Number($('i-threshold').value);
    const grey = sampleImage(sourceImage, width, height);

    if (!$('i-dither').checked) {
        for (let i = 0; i < grey.length; i++) pixels[i] = grey[i] < threshold ? 1 : 0;
    } else {
        // Floyd-Steinberg: push each pixel's rounding error onto its neighbours
        // so large flat areas keep their apparent shade.
        for (let y = 0; y < height; y++) {
            for (let x = 0; x < width; x++) {
                const i = y * width + x;
                const old = grey[i];
                const on = old < threshold;
                pixels[i] = on ? 1 : 0;

                const error = old - (on ? 0 : 255);
                const spread = (dx, dy, factor) => {
                    const nx = x + dx;
                    const ny = y + dy;
                    if (nx < 0 || ny < 0 || nx >= width || ny >= height) return;
                    grey[ny * width + nx] += error * factor;
                };
                spread(1, 0, 7 / 16);
                spread(-1, 1, 3 / 16);
                spread(0, 1, 5 / 16);
                spread(1, 1, 1 / 16);
            }
        }
    }

    draw();
    status(`image applied at ${width}×${height}`, 'ok');
}

// --- wiring -----------------------------------------------------------------

async function refreshExisting() {
    const res = await fetch('/api/generated/shapes');
    const { names } = await res.json();
    $('i-load').innerHTML =
        '<option value="">Load icon…</option>' +
        names.map((n) => `<option value="${esc(n)}">${esc(n)}</option>`).join('');
}

function selectTool(next) {
    tool = next;
    for (const button of root.querySelectorAll('[data-tool]')) {
        button.setAttribute('aria-pressed', String(button.dataset.tool === next));
    }
}

function init(panelRoot) {
    root = panelRoot;
    const canvas = $('i-canvas');
    canvas.oncontextmenu = (e) => e.preventDefault();

    let painting = 0;
    canvas.onmousedown = (e) => {
        painting = e.button === 2 ? 2 : 1;
        paint(e, painting === 1 ? 1 : 0);
        e.preventDefault();
    };
    canvas.onmousemove = (e) => {
        if (painting) paint(e, painting === 1 ? 1 : 0);
    };
    addEventListener('mouseup', () => (painting = 0));

    $('i-resize').onclick = () => {
        resize(Math.max(1, Number($('i-width').value)), Math.max(1, Number($('i-height').value)));
        draw();
    };

    $('i-clear').onclick = () => {
        pixels.fill(0);
        draw();
    };

    $('i-invert').onclick = () => {
        for (let i = 0; i < pixels.length; i++) pixels[i] = pixels[i] ? 0 : 1;
        draw();
    };

    for (const button of root.querySelectorAll('[data-tool]')) {
        button.onclick = () => selectTool(button.dataset.tool);
    }

    // Number fields respond to the wheel, which is quicker than the spinners.
    for (const input of root.querySelectorAll('input[type="number"]')) {
        input.addEventListener(
            'wheel',
            (event) => {
                if (document.activeElement !== input) return;
                event.preventDefault();
                const step = event.deltaY < 0 ? 1 : -1;
                const min = Number(input.min || -Infinity);
                const max = Number(input.max || Infinity);
                input.value = Math.min(max, Math.max(min, Number(input.value) + step));
                input.dispatchEvent(new Event('change'));
            },
            { passive: false },
        );
    }

    $('i-radius').onchange = () => {};

    $('i-show-code').onchange = () => {
        $('i-code-pane').hidden = !$('i-show-code').checked;
        draw();
    };

    $('i-copy').onclick = async () => {
        try {
            await navigator.clipboard.writeText($('i-code').textContent);
            codeStatus('copied');
        } catch (err) {
            codeStatus('could not write to the clipboard', true);
        }
    };

    $('i-paste').onclick = async () => {
        try {
            const parsed = parseHeader(await navigator.clipboard.readText());
            if (!parsed) return codeStatus('clipboard holds no icon header', true);

            width = parsed.width;
            height = parsed.height;
            pixels = parsed.pixels;
            sourceImage = null;
            $('i-image-bar').hidden = true;
            $('i-width').value = width;
            $('i-height').value = height;
            draw();
            codeStatus(`pasted ${width}×${height}`);
        } catch (err) {
            codeStatus('could not read the clipboard', true);
        }
    };

    $('i-use-image-size').onclick = () => {
        if (!sourceImage) return;
        resize(sourceImage.width, sourceImage.height);
        applyImage();
    };

    $('i-zoom-in').onclick = () => {
        zoom = Math.min(40, (zoom || currentScale()) + 1);
        draw();
    };
    $('i-zoom-out').onclick = () => {
        zoom = Math.max(1, (zoom || currentScale()) - 1);
        draw();
    };
    $('i-zoom-fit').onclick = () => {
        zoom = 0;
        draw();
    };

    $('i-name').oninput = () => draw();

    $('i-threshold').oninput = () => {
        $('i-threshold-label').textContent = $('i-threshold').value;
    };
    $('i-threshold').onchange = applyImage;
    $('i-dither').onchange = applyImage;

    $('i-image').onchange = (event) => {
        const file = event.target.files[0];
        if (!file) return;

        const image = new Image();
        image.onload = () => {
            sourceImage = image;
            $('i-image-bar').hidden = false;
            $('i-image-info').textContent = `source ${image.width}×${image.height}`;
            applyImage();
        };
        image.onerror = () => status('could not read that image', 'bad');
        image.src = URL.createObjectURL(file);
    };

    $('i-load').onchange = async () => {
        const name = $('i-load').value;
        if (!name) return;

        const res = await fetch(`/api/generated/shapes/${name}`);
        if (!res.ok) return status('could not read that icon', 'bad');

        const parsed = parseHeader((await res.json()).contents);
        if (!parsed) return status('could not read that icon', 'bad');

        width = parsed.width;
        height = parsed.height;
        pixels = parsed.pixels;
        sourceImage = null;
        $('i-image-bar').hidden = true;
        $('i-width').value = width;
        $('i-height').value = height;
        $('i-name').value = name;
        draw();
        status(`loaded ${name}`, 'ok');
    };

    $('i-save').onclick = async () => {
        const name = $('i-name').value.trim();
        if (!name) return status('name it first', 'bad');

        const res = await fetch(`/api/generated/shapes/${name}`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ contents: generateCode() }),
        });
        const body = await res.json();
        if (!res.ok) return status(body.error || 'save failed', 'bad');

        status(`saved ${body.path}`, 'ok');
        refreshExisting();
    };

    addEventListener('resize', () => {
        if (zoom === 0) draw();
    });

    draw();
    refreshExisting();
}

export default {
    id: 'icons',
    title: 'Icons',
    init,
    onShow: draw,
};
