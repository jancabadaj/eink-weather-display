import { $, esc } from '../app.js';
import { showBuildOutput } from './preview.js';

// Tests are only built and run while this panel is open, so the preview loop
// stays fast when you are not looking at them.
function setActive(active) {
    fetch('/api/tests/active', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ active }),
    });
}

function init() {
    $('tests-run').onclick = () => fetch('/api/tests/run', { method: 'POST' });

    $('tests-auto').onchange = () =>
        fetch('/api/tests/autorun', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ enabled: $('tests-auto').checked }),
        });
}

function apply(t) {
    const state = t.tests || {};
    const res = state.results;
    const failedBuild = !!(state.errorLines && state.errorLines.length) || (!res && !state.running);

    $('tests-auto').checked = !!state.autoRun;

    showBuildOutput({ ok: !failedBuild, output: state.output, errorLines: state.errorLines }, 'tests-output');

    const errorLines = state.errorLines || [];
    $('t-status').textContent = state.running
        ? 'running…'
        : errorLines.length
          ? 'build failed'
          : res
            ? res.failed
                ? 'failing'
                : 'passing'
            : 'not run';
    $('t-status').style.color = state.ok
        ? 'var(--good)'
        : errorLines.length || (res && res.failed)
          ? 'var(--bad)'
          : 'var(--muted)';
    $('t-passed').textContent = res ? res.passed : '—';
    $('t-failed').textContent = res ? res.failed : '—';
    $('t-duration').textContent = state.lastRunMs ? state.lastRunMs + ' ms' : '—';

    $('tests-run').disabled = !!state.running;

    if (!res) {
        $('tests-list').innerHTML =
            '<div class="row"><span>' +
            (state.running ? 'Running…' : 'Open this tab or press Run to execute the suite.') +
            '</span></div>';
        return;
    }

    let suite = null;
    const rows = [];
    for (const tc of res.tests) {
        if (tc.suite !== suite) {
            suite = tc.suite;
            rows.push(`<h2 class="section" style="margin-top:14px">${esc(suite || 'tests')}</h2>`);
        }
        rows.push(
            `<div class="test-row ${tc.ok ? 'pass' : 'fail'}">` +
                `<span class="mark">${tc.ok ? '●' : '✕'}</span>` +
                `<span class="nm">${esc(tc.name)}</span>` +
                `<span class="ms">${tc.asserts.passed} asserts</span></div>`
        );
        for (const f of tc.failures) {
            rows.push(
                `<div class="test-fail-detail">${esc(f.kind)}  ${esc(f.original)}` +
                    (f.expanded ? `\nwith  ${esc(f.expanded)}` : '') +
                    `\n<span class="loc">${esc(f.file)}:${f.line}</span></div>`
            );
        }
    }
    $('tests-list').innerHTML = rows.join('');
}

// Failures only. A green tick would imply the suite is continuously green,
// but tests do not run while this panel is closed.
function badge(s) {
    const t = (s && s.tests) || {};
    if (t.errorLines && t.errorLines.length) return { text: '!', cls: 'fail' };
    if (t.results && t.results.failed) return { text: String(t.results.failed), cls: 'fail' };
    return { text: '', cls: '' };
}

export default {
    id: 'tests',
    title: 'Tests',
    init,
    apply,
    badge,
    onShow: () => setActive(true),
    onHide: () => setActive(false),
};
