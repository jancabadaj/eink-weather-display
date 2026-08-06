'use strict';

// Reads and writes the generated headers the panels produce.
//
// Writes are confined to the two directories that hold generated art, and the
// name must be a plain identifier, so a panel cannot place a file anywhere else
// in the tree.

const fs = require('fs');
const path = require('path');

const ROOT = path.resolve(__dirname, '../../..');

const TARGETS = {
    shapes: path.join(ROOT, 'src', 'render', 'shapes'),
    fonts: path.join(ROOT, 'src', 'render', 'fonts'),
};

const SAFE_NAME = /^[A-Za-z0-9_-]+$/;

function resolveTarget(kind, name) {
    const dir = TARGETS[kind];
    if (!dir) throw new Error(`unknown target "${kind}"`);
    if (!SAFE_NAME.test(name)) throw new Error(`"${name}" is not a valid file name`);

    const file = path.join(dir, `${name}.h`);
    if (path.dirname(path.resolve(file)) !== dir) {
        throw new Error('refusing to write outside the generated directory');
    }
    return file;
}

function list(kind) {
    const dir = TARGETS[kind];
    if (!dir || !fs.existsSync(dir)) return [];
    return fs
        .readdirSync(dir)
        .filter((f) => f.endsWith('.h'))
        .map((f) => f.slice(0, -2))
        .sort();
}

function read(kind, name) {
    const file = resolveTarget(kind, name);
    return fs.existsSync(file) ? fs.readFileSync(file, 'utf8') : null;
}

function write(kind, name, contents) {
    const file = resolveTarget(kind, name);
    fs.writeFileSync(file, contents);
    return path.relative(ROOT, file);
}

module.exports = { list, read, write };
