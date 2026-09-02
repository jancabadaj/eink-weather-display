import { $ } from '../app.js';

// The panel's own root; panels share a document, so queries stay inside it.
let root = document;

// Rasterises a TTF and packs it the way DrawUtils reads it: MSB-first, each
// glyph row padded to whole bytes.
//
// Glyph width is the font's own advance plus a pixel of padding each side, and the
// baseline is derived from the font's own ascent and descent - the same rules
// the headers already in src/render/fonts were produced under, so a regenerated
// font keeps the metrics the layout was designed around.

const FIRST_CHAR = 32;
const LAST_CHAR = 126;

let family = null;
let glyphs = [];
let baseline = 0;
let mode = 'proportional';

function status(text, cls = '') {
    $('f-status').textContent = text;
    $('f-status').className = cls;
}

function codeStatus(text, bad = false) {
    $('f-code-status').textContent = text;
    $('f-code-status').style.color = bad ? 'var(--bad)' : 'var(--good)';
}

function fontSpec(height, bold) {
    return `${bold ? 'bold ' : ''}${height}px "${family}"`;
}

// Places the baseline so ascent and descent are centred in the glyph box.
function measureBaseline(height, bold) {
    const ctx = document.createElement('canvas').getContext('2d');
    ctx.font = fontSpec(height, bold);

    const metrics = ctx.measureText('ÁgjpqyÖ|');
    let ascent = Math.ceil(metrics.actualBoundingBoxAscent || height * 0.75);
    let descent = Math.ceil(metrics.actualBoundingBoxDescent || height * 0.25);

    const total = ascent + descent;
    if (total > height && total > 0) {
        const scale = height / total;
        ascent = Math.floor(ascent * scale);
        descent = Math.floor(descent * scale);
    }

    const marginTop = Math.max(1, Math.floor((height - ascent - descent) / 2));
    let line = marginTop + ascent;
    if (line + descent > height) line = height - descent - 1;
    if (line < ascent) line = ascent;
    if (line < 0) line = Math.floor(height * 0.75);
    return line;
}

// Draws one character into a box as wide as the glyph's advance.
//
// DrawUtils steps the pen by the stored width, so the width is what spaces
// letters apart. Taking it from the advance keeps each glyph's side bearings,
// which is the spacing the typeface was designed with; measuring the ink
// instead would pull every letter tight against its neighbours. `padding` adds
// blank columns either side on top of that.
//
// The canvas rasteriser antialiases, so `threshold` decides how much of a
// glyph's soft edge counts as ink - higher values keep more of it.
function rasterise(ch, height, threshold, bold, padding) {
    const pad = Math.ceil(height * 1.5);
    const canvas = document.createElement('canvas');
    canvas.width = Math.ceil(height * 3) + pad * 2;
    canvas.height = height + pad * 2;

    const ctx = canvas.getContext('2d', { willReadFrequently: true });
    ctx.fillStyle = '#fff';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.fillStyle = '#000';
    ctx.textBaseline = 'alphabetic';
    ctx.font = fontSpec(height, bold);
    ctx.fillText(ch, pad, pad + baseline);

    const image = ctx.getImageData(0, 0, canvas.width, canvas.height).data;
    const inked = (x, y) => {
        const sx = pad + x;
        const sy = pad + y;
        if (sx < 0 || sy < 0 || sx >= canvas.width || sy >= canvas.height) return 0;
        return image[(sy * canvas.width + sx) * 4] < threshold ? 1 : 0;
    };

    // The box spans the advance run and the ink together. The advance is what
    // sets the spacing, but diagonals and tails - A, V, j, / - put ink outside
    // it, and a box sized on the advance alone would shave them off.
    const metrics = ctx.measureText(ch);
    const start = Math.min(0, Math.floor(-metrics.actualBoundingBoxLeft));
    const end = Math.max(metrics.width, Math.ceil(metrics.actualBoundingBoxRight));
    const width = Math.max(1, Math.round(end - start)) + padding * 2;

    // Keep the pen where the box says it is, so the glyph holds its bearings.
    const left = start - padding;

    const rows = [];
    for (let y = 0; y < height; y++) {
        const row = [];
        for (let x = 0; x < width; x++) row.push(inked(left + x, y));
        rows.push(row);
    }
    return { ch, width, rows };
}

function pack(glyph) {
    const perRow = Math.ceil(glyph.width / 8);
    const bytes = [];
    for (const row of glyph.rows) {
        const packed = new Uint8Array(perRow);
        for (let x = 0; x < glyph.width; x++) {
            if (row[x]) packed[x >> 3] |= 0x80 >> (x & 7);
        }
        bytes.push(...packed);
    }
    return bytes;
}

// Widens every glyph to a common box, centring what is already there.
function toMonospace(source) {
    const width = Math.max(...source.map((g) => g.width));
    return source.map((glyph) => {
        const pad = Math.floor((width - glyph.width) / 2);
        return {
            ch: glyph.ch,
            width,
            rows: glyph.rows.map((row) => {
                const padded = new Array(width).fill(0);
                row.forEach((on, x) => (padded[x + pad] = on));
                return padded;
            }),
        };
    });
}

function generateMonospaceCode(name, height, sourceName) {
    const boxed = toMonospace(glyphs);
    const width = boxed[0].width;
    const perRow = Math.ceil(width / 8);

    const hex = (b) => '0x' + b.toString(16).toUpperCase().padStart(2, '0');
    const lines = [];
    let total = 0;

    boxed.forEach((glyph) => {
        const label = glyph.ch === '\\' ? '\\\\' : glyph.ch;
        lines.push(`    // '${label}'`);
        for (const row of glyph.rows) {
            const packed = new Uint8Array(perRow);
            for (let x = 0; x < width; x++) {
                if (row[x]) packed[x >> 3] |= 0x80 >> (x & 7);
            }
            let art = '';
            for (let x = 0; x < width; x++) art += row[x] ? '#' : ' ';
            lines.push(`    ${Array.from(packed).map(hex).join(', ')}, //${art}`);
            total += perRow;
        }
    });

    $('f-metrics').textContent = `width ${width}px · baseline ${baseline}px · ${total} bytes`;

    return (
        `/**\n * Font generated from ${sourceName}\n` +
        ` * Height: ${height} pixels\n * Type: Monospace\n` +
        ` * Characters: ASCII ${FIRST_CHAR}-${LAST_CHAR}\n` +
        ` * Baseline: ${baseline} pixels from top\n */\n\n` +
        `#pragma once\n\n#include "../shape.h"\n\n` +
        `const uint8_t ${name}_Table[] =\n{\n${lines.join('\n')}\n};\n\n` +
        `Shape ${name} = {\n    ${name}_Table,\n    ${width}, /* Width */\n` +
        `    ${height}, /* Height */\n};\n`
    );
}

function generateCode(name, height, sourceName) {
    if (mode === 'monospace') {
        return generateMonospaceCode(name, height, sourceName);
    }

    const table = [];
    const widths = [];
    const offsets = [];
    let maxWidth = 0;

    for (const glyph of glyphs) {
        offsets.push(table.length);
        widths.push(glyph.width);
        maxWidth = Math.max(maxWidth, glyph.width);
        table.push(...pack(glyph));
    }

    const hex = (b) => '0x' + b.toString(16).toUpperCase().padStart(2, '0');
    const wrap = (values, perLine) => {
        const lines = [];
        for (let i = 0; i < values.length; i += perLine) {
            lines.push('    ' + values.slice(i, i + perLine).join(', ') + ',');
        }
        return lines.join('\n');
    };

    const tableLines = [];
    let at = 0;
    glyphs.forEach((glyph, index) => {
        const perRow = Math.ceil(glyph.width / 8);
        const label = glyph.ch === '\\' ? '\\\\' : glyph.ch;
        tableLines.push(`    // @${offsets[index]} '${label}' (${glyph.width}px wide)`);
        for (const row of glyph.rows) {
            const bytes = table.slice(at, at + perRow);
            let art = '';
            for (let x = 0; x < glyph.width; x++) art += row[x] ? '#' : ' ';
            tableLines.push(`    ${bytes.map(hex).join(', ')}, //${art}`);
            at += perRow;
        }
    });

    const average = (widths.reduce((sum, w) => sum + w, 0) / widths.length).toFixed(1);
    $('f-metrics').textContent =
        `max ${maxWidth}px · avg ${average}px · baseline ${baseline}px · ${table.length} bytes`;

    return (
        `/**\n * Font generated from ${sourceName}\n` +
        ` * Height: ${height} pixels\n * Type: Proportional\n` +
        ` * Characters: ASCII ${FIRST_CHAR}-${LAST_CHAR}\n` +
        ` * Baseline: ${baseline} pixels from top\n */\n\n` +
        `#pragma once\n\n#include "../proportionalFont.h"\n\n` +
        `const uint8_t ${name}_Table[] =\n{\n${tableLines.join('\n')}\n};\n\n` +
        `const uint16_t ${name}_Widths[] = {\n${wrap(widths, 16)}\n};\n\n` +
        `const uint32_t ${name}_Offsets[] = {\n${wrap(offsets, 8)}\n};\n\n` +
        `ProportionalFont ${name} = {\n    ${name}_Table,\n    ${name}_Widths,\n` +
        `    ${name}_Offsets,\n    ${maxWidth}, /* Max width */\n` +
        `    ${height}, /* Height */\n    ${FIRST_CHAR}, /* First char */\n};\n`
    );
}

// Reads a generated header back into glyphs, so an existing font file can be
// opened and looked at without the TTF it was made from.
function parseHeader(text) {
    const symbol = (text.match(/^(?:ProportionalFont|Shape)\s+(\w+)\s*=/m) || [])[1];
    if (!symbol) return null;

    const isMono = /^Shape\s+\w+\s*=/m.test(text);
    const height = Number((text.match(/(\d+),\s*\/\*\s*Height\s*\*\//) || [])[1]);
    if (!height) return null;

    // The byte rows carry ASCII art in trailing comments; drop those first so
    // only the literals are left to scan.
    const body = text.replace(/\/\/[^\n]*/g, '');
    const tableSection = (body.match(/_Table\[\]\s*=\s*\{([\s\S]*?)\};/) || [])[1];
    if (!tableSection) return null;

    const bytes = (tableSection.match(/0x[0-9A-Fa-f]{2}/g) || []).map((b) => parseInt(b, 16));
    if (!bytes.length) return null;

    let widths;
    if (isMono) {
        const width = Number((text.match(/(\d+),\s*\/\*\s*Width\s*\*\//) || [])[1]);
        if (!width) return null;
        widths = new Array(Math.floor(bytes.length / (Math.ceil(width / 8) * height))).fill(width);
    } else {
        const section = (body.match(/_Widths\[\]\s*=\s*\{([\s\S]*?)\};/) || [])[1];
        if (!section) return null;
        widths = (section.match(/\d+/g) || []).map(Number);
    }
    if (!widths.length) return null;

    const parsed = [];
    let at = 0;
    for (let index = 0; index < widths.length; index++) {
        const width = widths[index];
        const perRow = Math.ceil(width / 8);
        if (at + perRow * height > bytes.length) return null;

        const rows = [];
        for (let y = 0; y < height; y++) {
            const row = [];
            for (let x = 0; x < width; x++) {
                row.push((bytes[at + (x >> 3)] >> (7 - (x & 7))) & 1);
            }
            rows.push(row);
            at += perRow;
        }
        parsed.push({ ch: String.fromCharCode(FIRST_CHAR + index), width, rows });
    }

    return {
        isMono,
        height,
        glyphs: parsed,
        baseline: Number((text.match(/Baseline:\s*(\d+)/) || [])[1]) || Math.round(height * 0.8),
        fontId: symbol.replace(/^Font\d+_/, '').replace(/_(?:Proportional|Monospace)$/, ''),
    };
}

function drawPreview(height) {
    const canvas = $('f-canvas');
    const scale = 2;
    const perLine = 24;
    const lines = Math.ceil(glyphs.length / perLine);
    const cell = Math.max(...glyphs.map((g) => g.width)) + 2;

    canvas.width = perLine * cell * scale;
    canvas.height = lines * (height + 4) * scale;

    const ctx = canvas.getContext('2d');
    ctx.fillStyle = '#fff';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.fillStyle = '#000';

    glyphs.forEach((glyph, index) => {
        const ox = (index % perLine) * cell * scale;
        const oy = Math.floor(index / perLine) * (height + 4) * scale;
        glyph.rows.forEach((pixels, y) => {
            pixels.forEach((on, x) => {
                if (on) ctx.fillRect(ox + x * scale, oy + y * scale, scale, scale);
            });
        });
    });
}

let sourceName = 'unknown.ttf';
let fontId = 'CustomFont';

// Reads the typeface's own PostScript name out of the sfnt `name` table, so the
// generated identifiers follow the font rather than whatever the file on disk
// happens to be called.
function readPostScriptName(buffer) {
    const view = new DataView(buffer);
    const bytes = new Uint8Array(buffer);

    let table = 0;
    for (let i = 0; i < view.getUint16(4); i++) {
        const entry = 12 + i * 16;
        const tag = String.fromCharCode(...bytes.subarray(entry, entry + 4));
        if (tag === 'name') table = view.getUint32(entry + 8);
    }
    if (!table) return null;

    const count = view.getUint16(table + 2);
    const strings = table + view.getUint16(table + 4);
    const found = {};

    for (let i = 0; i < count; i++) {
        const record = table + 6 + i * 12;
        const platform = view.getUint16(record);
        const nameId = view.getUint16(record + 6);
        const length = view.getUint16(record + 8);
        const offset = view.getUint16(record + 10);

        const raw = bytes.subarray(strings + offset, strings + offset + length);
        let text = '';
        if (platform === 3) {
            for (let c = 0; c + 1 < raw.length; c += 2)
                text += String.fromCharCode((raw[c] << 8) | raw[c + 1]);
        } else {
            text = String.fromCharCode(...raw);
        }
        // 6 is the PostScript name, 4 the full name, 1 and 2 family and style.
        if ([1, 2, 4, 6].includes(nameId) && !found[nameId]) found[nameId] = text;
    }

    return found[6] || found[4] || [found[1], found[2]].filter(Boolean).join('-') || null;
}

const sanitise = (text) => text.replace(/[^A-Za-z0-9]+/g, '_').replace(/^_+|_+$/g, '');

// Identifiers follow the headers already in src/render/fonts: the struct is
// Font58_Roboto_BoldCondensed_Proportional, the file font58-prop.h.
function symbolName(height) {
    return `Font${height}_${fontId}_${mode === 'monospace' ? 'Monospace' : 'Proportional'}`;
}

function fileName(height) {
    return `font${height}-${mode === 'monospace' ? 'mono' : 'prop'}`;
}

function setMode(next) {
    mode = next;
    for (const button of root.querySelectorAll('[data-mode]')) {
        button.setAttribute('aria-pressed', String(button.dataset.mode === next));
    }
}

function rebuild() {
    if (!family) return;

    const height = Math.max(6, Number($('f-height').value));
    const threshold = Number($('f-threshold').value);
    const bold = $('f-bold').checked;
    const padding = Math.max(0, Number($('f-padding').value));

    baseline = measureBaseline(height, bold);

    glyphs = [];
    for (let code = FIRST_CHAR; code <= LAST_CHAR; code++) {
        glyphs.push(rasterise(String.fromCharCode(code), height, threshold, bold, padding));
    }

    drawPreview(height);
    $('f-name').value = fileName(height);
    $('f-code').textContent = generateCode(symbolName(height), height, sourceName);
    status(`${glyphs.length} glyphs rasterised`, 'ok');
}

function init(panelRoot) {
    root = panelRoot;
    $('f-file').onchange = async (event) => {
        const file = event.target.files[0];
        if (!file) return;

        try {
            const buffer = await file.arrayBuffer();
            family = 'wb_' + file.name.replace(/[^A-Za-z0-9]/g, '_');
            sourceName = file.name;
            fontId = sanitise(readPostScriptName(buffer) || file.name.replace(/\.[^.]+$/, ''));
            const face = new FontFace(family, buffer);
            await face.load();
            document.fonts.add(face);
            rebuild();
        } catch (err) {
            status('could not load that font: ' + err.message, 'bad');
        }
    };

    $('f-threshold').oninput = () => {
        $('f-threshold-label').textContent = $('f-threshold').value;
    };
    // The name follows the inputs, so it is not itself an input to the build.
    for (const id of ['f-height', 'f-threshold', 'f-bold', 'f-padding']) {
        $(id).onchange = rebuild;
    }

    $('f-show-code').onchange = () => {
        $('f-code-pane').hidden = !$('f-show-code').checked;
    };

    $('f-copy').onclick = async () => {
        try {
            await navigator.clipboard.writeText($('f-code').textContent);
            codeStatus('copied');
        } catch (err) {
            codeStatus('could not write to the clipboard', true);
        }
    };

    $('f-paste').onclick = async () => {
        try {
            const parsed = parseHeader(await navigator.clipboard.readText());
            if (!parsed) return codeStatus('clipboard holds no font header', true);

            glyphs = parsed.glyphs;
            baseline = parsed.baseline;
            fontId = parsed.fontId;
            setMode(parsed.isMono ? 'monospace' : 'proportional');
            $('f-height').value = parsed.height;

            drawPreview(parsed.height);
            $('f-name').value = fileName(parsed.height);
            $('f-code').textContent = generateCode(symbolName(parsed.height), parsed.height, sourceName);
            codeStatus(`pasted ${glyphs.length} glyphs at ${parsed.height}px`);
        } catch (err) {
            codeStatus('could not read the clipboard', true);
        }
    };

    for (const button of root.querySelectorAll('[data-mode]')) {
        button.onclick = () => {
            setMode(button.dataset.mode);
            rebuild();
        };
    }

    $('f-save').onclick = async () => {
        const name = $('f-name').value.trim();
        if (!name) return status('name it first', 'bad');
        if (!glyphs.length) return status('load a font first', 'bad');

        const res = await fetch(`/api/generated/fonts/${name}`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ contents: $('f-code').textContent }),
        });
        const body = await res.json();
        if (!res.ok) return status(body.error || 'save failed', 'bad');
        status(`saved ${body.path}`, 'ok');
    };

    status('choose a .ttf to begin');
}

export default {
    id: 'fonts',
    title: 'Fonts',
    init,
};
