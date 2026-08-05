import { $ } from '../app.js';
import { showBuildOutput } from './preview.js';

let currentVersion = -1;
let applying = false;

function setActive(active) {
    fetch('/api/admin/active', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ active }),
    });
}

function pushSnapshot() {
    if (applying) return; // echoing server state back at it would loop
    fetch('/api/admin/snapshot', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
            loggedIn: $('a-loggedin').checked,
            updatesStopped: $('a-stopped').checked,
            nightOverridden: $('a-override').checked,
            nightStartHour: Number($('a-night-start').value),
            nightEndHour: Number($('a-night-end').value),
            localAddress: $('a-ip').value,
        }),
    });
}

function init() {
    for (const id of ['a-loggedin', 'a-stopped', 'a-override']) {
        $(id).onchange = pushSnapshot;
    }
    for (const id of ['a-night-start', 'a-night-end', 'a-ip']) {
        $(id).onchange = pushSnapshot;
    }
}

function apply(s) {
    const admin = s.admin || {};
    const snapshot = admin.snapshot || {};

    applying = true;
    $('a-loggedin').checked = !!snapshot.loggedIn;
    $('a-stopped').checked = !!snapshot.updatesStopped;
    $('a-override').checked = !!snapshot.nightOverridden;
    $('a-night-start').value = snapshot.nightStartHour ?? 21;
    $('a-night-end').value = snapshot.nightEndHour ?? 5;
    $('a-ip').value = snapshot.localAddress ?? '';
    applying = false;

    showBuildOutput(
        { ok: admin.ok, output: admin.output, errorLines: admin.errorLines },
        'admin-output'
    );

    $('a-status').textContent = admin.ok ? 'rendered' : admin.version ? 'failed' : 'not rendered';
    $('a-status').style.color = admin.ok ? 'var(--good)' : 'var(--muted)';
    $('a-version').textContent = admin.version ? '#' + admin.version : '—';

    if (admin.version && admin.version !== currentVersion) {
        currentVersion = admin.version;
        $('admin-page').src = '/api/admin.html?v=' + admin.version;
    }
}

export default {
    id: 'admin',
    title: 'Admin',
    init,
    apply,
    badge: (s) => (s && s.admin && s.admin.errorLines && s.admin.errorLines.length ? { text: '!', cls: 'fail' } : { text: '', cls: '' }),
    onShow: () => setActive(true),
    onHide: () => setActive(false),
};
