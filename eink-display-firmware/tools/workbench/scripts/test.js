#!/usr/bin/env node
'use strict';

// Builds and runs the host test suite.  Usage:  npm test
// Exits non-zero if the build fails or any test fails, so it works as a CI gate.
//
// Compiler output is always printed verbatim - warnings included, and whatever
// the build emitted when it failed - so nothing can hide behind a summary.

const { buildTests, runTests } = require('../server/build');

const dim = (s) => `\x1b[2m${s}\x1b[0m`;
const red = (s) => `\x1b[31m${s}\x1b[0m`;
const green = (s) => `\x1b[32m${s}\x1b[0m`;
const bold = (s) => `\x1b[1m${s}\x1b[0m`;

async function main() {
    const built = await buildTests();

    if (built.output.trim()) {
        console.log(dim('--- compiler output ---'));
        console.log(built.output.trimEnd());
        console.log(dim('--- end compiler output ---\n'));
    }

    if (!built.ok) {
        console.error(red('Build failed.'));
        process.exit(1);
    }

    const run = await runTests();
    if (!run.ok) {
        console.error(red('Could not run tests:'));
        console.error(run.error);
        process.exit(1);
    }

    const { tests, passed, failed, durationMs } = run.results;

    let suite = null;
    for (const t of tests) {
        if (t.suite !== suite) {
            suite = t.suite;
            console.log(`\n${bold(suite || '(no suite)')}`);
        }
        console.log(
            `  ${t.ok ? green('✓') : red('✗')} ${t.name} ${dim(`(${t.asserts.passed} asserts)`)}`
        );
        for (const f of t.failures) {
            console.log(red(`      ${f.kind}  ${f.original}`));
            if (f.expanded) console.log(red(`      with   ${f.expanded}`));
            console.log(dim(`      at     ${f.file}:${f.line}`));
        }
    }

    const summary = `${passed} passed, ${failed} failed  ${dim(`(${durationMs} ms)`)}`;
    console.log(`\n${failed ? red(summary) : green(summary)}\n`);
    process.exit(failed ? 1 : 0);
}

main().catch((err) => {
    console.error(err);
    process.exit(1);
});
