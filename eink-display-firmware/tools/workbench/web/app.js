// Workbench shell: loads panels, owns the tab bar and the websocket, and hands
// server state to whichever panels care about it.
//
// A panel is a folder-mate trio - panels/<id>.html / .css / .js - where the JS
// default-exports { id, title, init, apply, onShow, onHide, badge }. Adding a
// panel means adding those three files and one entry in PANELS.

const PANELS = ['preview', 'admin', 'tests'];
const PLACEHOLDERS = ['Icons', 'Fonts'];

export const $ = (id) => document.getElementById(id);
export const esc = (t) =>
    String(t).replace(/[&<>]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;' })[c]);

const modules = new Map();
let active = null;
let lastState = null;

function select(id) {
    if (active === id) return;
    if (active) {
        $('panel-' + active).hidden = true;
        modules.get(active).onHide?.();
    }
    active = id;
    $('panel-' + id).hidden = false;
    for (const tab of document.querySelectorAll('.tab[data-panel]')) {
        tab.setAttribute('aria-selected', String(tab.dataset.panel === id));
    }
    modules.get(id).onShow?.(lastState);
}

export function isActive(id) {
    return active === id;
}

function applyAll(state) {
    lastState = state;
    for (const [id, mod] of modules) {
        mod.apply?.(state);
        const badge = mod.badge?.(state) || { text: '', cls: '' };
        const el = $('badge-' + id);
        el.textContent = badge.text;
        el.className = 'badge ' + (badge.cls || '');
    }
}

function connect() {
    const ws = new WebSocket(`ws://${location.host}`);
    ws.onopen = () => {
        $('conn').className = 'live';
        $('conn').textContent = 'live';
    };
    ws.onmessage = (e) => {
        const msg = JSON.parse(e.data);
        if (msg.type === 'state') applyAll(msg.state);
    };
    ws.onclose = () => {
        $('conn').className = 'dead';
        $('conn').textContent = 'reconnecting';
        setTimeout(connect, 700);
    };
}

async function boot() {
    const nav = $('nav');
    const host = $('panels');

    for (const id of PANELS) {
        document.head.insertAdjacentHTML('beforeend', `<link rel="stylesheet" href="panels/${id}.css">`);

        const html = await (await fetch(`panels/${id}.html`)).text();
        const main = document.createElement('main');
        main.id = 'panel-' + id;
        main.hidden = true;
        main.innerHTML = html;
        host.appendChild(main);

        const mod = (await import(`./panels/${id}.js`)).default;
        modules.set(id, mod);

        const tab = document.createElement('div');
        tab.className = 'tab';
        tab.dataset.panel = id;
        tab.setAttribute('role', 'tab');
        tab.innerHTML = `${mod.title} <span class="badge" id="badge-${id}"></span>`;
        tab.onclick = () => select(id);
        nav.appendChild(tab);

        mod.init?.();
    }

    for (const name of PLACEHOLDERS) {
        const tab = document.createElement('div');
        tab.className = 'tab';
        tab.setAttribute('disabled', '');
        tab.textContent = name;
        nav.appendChild(tab);
    }

    select(PANELS[0]);
    connect();
}

boot();
