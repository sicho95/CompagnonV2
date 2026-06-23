const SERVICE_UUID = '12345678-1234-1234-1234-1234567890ab';
const CHAR_UUIDS = {
  wifiScan: '00002002-0000-1000-8000-00805f9b34fb',
  wifiProvision: '00002003-0000-1000-8000-00805f9b34fb',
  agentSync: '00002004-0000-1000-8000-00805f9b34fb',
  textInput: '00002005-0000-1000-8000-00805f9b34fb',
  llmRelay: '00002006-0000-1000-8000-00805f9b34fb',
  deviceStatus: '00002007-0000-1000-8000-00805f9b34fb',
  gps: '00002008-0000-1000-8000-00805f9b34fb'
};

const STORE_KEY = 'compagnonv2.pwa.state.v3';
const OLD_STORE_KEYS = ['compagnonv2.pwa.state.v2'];

const VIEW_META = {
  nestor: ['Application', 'Nestor'],
  rappels: ['Application', 'Rappels'],
  meteo: ['Application', 'Meteo'],
  bourse: ['Application', 'Bourse'],
  radars: ['Application', 'Radars'],
  domotique: ['Application', 'Domotique'],
  ecovacs: ['Application', 'Ecovacs'],
  settings: ['Systeme', 'Parametres']
};

const SETTINGS_TABS = [
  ['global', 'Global'],
  ['esp32', 'ESP32'],
  ['nestor', 'Nestor & Agents'],
  ['rappels', 'Rappels'],
  ['meteo', 'Meteo'],
  ['bourse', 'Bourse'],
  ['radars', 'Radars'],
  ['domotique', 'Domotique'],
  ['ecovacs', 'Ecovacs'],
  ['apis', 'APIs'],
  ['sync', 'Sync']
];

const DEFAULT_STATE = {
  view: 'nestor',
  settingsTab: 'global',
  theme: 'auto',
  device: { connected: false, name: '', status: {}, lastSync: '', wifiResults: [] },
  config: {
    wifi: { ssid: '', pass: '' },
    system: {
      bleName: 'Compagnon',
      timezone: 'CET-1CEST,M3.5.0,M10.5.0/3',
      wakeWord: 'Nestor',
      volume: 70,
      autoSync: true,
      phoneGpsRelay: false
    },
    keys: {
      groq_key: '',
      gemini_key: '',
      serper_key: '',
      openrtr_key: '',
      twdata_key: '',
      meteo_key: '',
      spotify_id: '',
      spotify_sec: '',
      tuya_id: '',
      tuya_sec: '',
      tuya_region: 'eu',
      tuya_user: '',
      ecovacs_u: '',
      ecovacs_p: '',
      ecovacs_cc: 'fr',
      ecovacs_dev: ''
    },
    apps: {
      nestor: true,
      rappels: true,
      meteo: true,
      bourse: true,
      radars: true,
      domotique: true,
      ecovacs: true
    }
  },
  agents: [
    {
      id: 'agent-nestor',
      name: 'Nestor',
      role: 'orchestrator',
      model: 'llama-3.1-8b-instant',
      prompt: 'Tu es Nestor, assistant personnel de Damien pour CompagnonV2. Tu routes vers les apps et tu restes coherent avec SichoBrain quand le contexte est fourni.',
      updatedAt: ''
    },
    {
      id: 'agent-domotique',
      name: 'Domotique',
      role: 'maison',
      model: 'llama-3.1-8b-instant',
      prompt: 'Tu aides a piloter Tuya, SmartLife et Ecovacs depuis CompagnonV2.',
      updatedAt: ''
    }
  ],
  activeAgentId: 'agent-nestor',
  chat: [],
  reminders: [],
  bourse: { tickers: ['AAPL', 'BTC/USD', 'MC.PA', 'CAC:INDX'], quotes: [], updatedAt: '' },
  meteo: { lat: 48.8566, lon: 2.3522, city: 'Paris', forecast: [], updatedAt: '' },
  radar: {
    running: false,
    source: 'lufop',
    proxy: '',
    maxRadars: 5000,
    radiusKm: 1000,
    zoom: 8,
    speed: 0,
    pos: null,
    heading: 0,
    radars: [],
    nearest: null,
    updatedAt: ''
  },
  logs: []
};

let state = loadState();
let device = null;
let chars = {};
let phoneGpsWatch = null;
let radarWatch = null;
let radarAudio = null;
let syncTimer = null;
let pendingSync = false;

const $ = selector => document.querySelector(selector);
const $$ = selector => Array.from(document.querySelectorAll(selector));
const enc = value => new TextEncoder().encode(typeof value === 'string' ? value : JSON.stringify(value));
const dec = value => new TextDecoder().decode(value.buffer);

function merge(base, over) {
  const out = Array.isArray(base) ? [...base] : { ...base };
  for (const key of Object.keys(over || {})) {
    out[key] = base[key] && typeof base[key] === 'object' && !Array.isArray(base[key])
      ? merge(base[key], over[key])
      : over[key];
  }
  return out;
}

function loadState() {
  try {
    const raw = localStorage.getItem(STORE_KEY) || OLD_STORE_KEYS.map(k => localStorage.getItem(k)).find(Boolean) || '{}';
    const loaded = merge(DEFAULT_STATE, JSON.parse(raw));
    if (loaded.view === 'config') loaded.view = 'settings';
    return loaded;
  } catch {
    return structuredClone(DEFAULT_STATE);
  }
}

function saveState({ sync = true } = {}) {
  localStorage.setItem(STORE_KEY, JSON.stringify(state));
  if (sync && state.config.system.autoSync) scheduleSync();
}

function log(message) {
  const line = `${new Date().toLocaleTimeString()}  ${message}`;
  state.logs.unshift(line);
  state.logs = state.logs.slice(0, 120);
  localStorage.setItem(STORE_KEY, JSON.stringify(state));
}

function activeAgent() {
  return state.agents.find(agent => agent.id === state.activeAgentId) || state.agents[0];
}

function setView(view) {
  state.view = view;
  saveState({ sync: false });
  render();
}

function setSettingsTab(tab) {
  state.settingsTab = tab;
  saveState({ sync: false });
  render();
}

function applyTheme() {
  document.documentElement.dataset.theme = state.theme || 'auto';
  const themeMeta = document.querySelector('meta[name="theme-color"]');
  if (themeMeta) themeMeta.content = state.theme === 'dark' ? '#0b111c' : '#f4f7fb';
  const select = $('#themeSelect');
  if (select) select.value = state.theme || 'auto';
}

function render() {
  applyTheme();
  const [kicker, title] = VIEW_META[state.view] || VIEW_META.nestor;
  $('#viewKicker').textContent = kicker;
  $('#viewTitle').textContent = title;
  $$('.nav-section button').forEach(button => button.classList.toggle('active', button.dataset.view === state.view));
  renderBleState();
  const root = $('#view');
  root.innerHTML = '';
  const renderers = {
    nestor: renderNestor,
    rappels: renderRappels,
    meteo: renderMeteo,
    bourse: renderBourse,
    radars: renderRadars,
    domotique: renderDomotique,
    ecovacs: renderEcovacs,
    settings: renderSettings
  };
  (renderers[state.view] || renderNestor)(root);
}

function panel(title, opts = {}) {
  const node = document.createElement('section');
  node.className = `panel ${opts.className || ''}`.trim();
  const head = document.createElement('div');
  head.className = 'panel-head';
  const left = document.createElement('div');
  const h = document.createElement('h2');
  h.textContent = title;
  left.append(h);
  if (opts.subtitle) {
    const sub = document.createElement('p');
    sub.className = 'panel-subtitle';
    sub.textContent = opts.subtitle;
    left.append(sub);
  }
  head.append(left);
  if (opts.actions) head.append(opts.actions);
  node.append(head);
  return node;
}

function appHero(title, subtitle, actionNode = null) {
  const node = document.createElement('div');
  node.className = 'app-hero';
  const left = document.createElement('div');
  left.innerHTML = `<h2>${escapeHtml(title)}</h2><p>${escapeHtml(subtitle)}</p>`;
  node.append(left);
  if (actionNode) node.append(actionNode);
  return node;
}

function actions(...buttons) {
  const node = document.createElement('div');
  node.className = 'actions';
  buttons.filter(Boolean).forEach(button => node.append(button));
  return node;
}

function button(label, onClick, className = '') {
  const b = document.createElement('button');
  b.type = 'button';
  b.textContent = label;
  b.className = className;
  b.addEventListener('click', onClick);
  return b;
}

function field(label, value, onInput, options = {}) {
  const node = document.createElement('label');
  node.className = `field ${options.inline ? 'inline' : ''}`.trim();
  const span = document.createElement('span');
  span.textContent = label;
  let input;
  if (options.select) {
    input = document.createElement('select');
    options.select.forEach(([value, text]) => {
      const opt = document.createElement('option');
      opt.value = value;
      opt.textContent = text;
      input.append(opt);
    });
  } else if (options.textarea) {
    input = document.createElement('textarea');
  } else {
    input = document.createElement('input');
    input.type = options.type || 'text';
  }
  if (options.type === 'checkbox') {
    input.checked = Boolean(value);
  } else {
    input.value = value ?? '';
  }
  if (options.placeholder) input.placeholder = options.placeholder;
  input.addEventListener('input', () => onInput(options.type === 'checkbox' ? input.checked : input.value));
  input.addEventListener('change', () => saveState());
  node.append(span, input);
  return node;
}

function metric(label, value, className = '') {
  const node = document.createElement('div');
  node.className = 'metric';
  node.innerHTML = `<span></span><strong></strong>`;
  node.querySelector('span').textContent = label;
  const strong = node.querySelector('strong');
  strong.textContent = value;
  if (className) strong.className = className;
  return node;
}

function renderBleState() {
  const pill = $('#bleState');
  pill.textContent = state.device.connected ? `BLE ${state.device.name || 'connecte'}` : 'BLE deconnecte';
  pill.classList.toggle('ok', state.device.connected);
  $('#connectBle').textContent = state.device.connected ? 'Deconnecter' : 'Connecter';
}

async function connectBle() {
  if (device?.gatt?.connected) {
    device.gatt.disconnect();
    return;
  }
  if (!navigator.bluetooth) throw new Error('Web Bluetooth indisponible sur ce navigateur.');
  device = await navigator.bluetooth.requestDevice({
    filters: [{ services: [SERVICE_UUID] }],
    optionalServices: [SERVICE_UUID]
  });
  device.addEventListener('gattserverdisconnected', onBleDisconnected);
  const server = await device.gatt.connect();
  const service = await server.getPrimaryService(SERVICE_UUID);
  chars = {};
  for (const [name, uuid] of Object.entries(CHAR_UUIDS)) {
    try { chars[name] = await service.getCharacteristic(uuid); }
    catch { log(`BLE caracteristique absente: ${name}`); }
  }
  await subscribe('deviceStatus', raw => {
    state.device.status = JSON.parse(raw);
    saveState({ sync: false });
    renderBleState();
  });
  await subscribe('agentSync', raw => log(`ESP32 ${raw}`));
  await subscribe('wifiScan', raw => {
    state.device.wifiResults = JSON.parse(raw);
    saveState({ sync: false });
    if (state.view === 'settings' && state.settingsTab === 'esp32') render();
  });
  state.device.connected = true;
  state.device.name = device.name || 'Compagnon';
  log('BLE connecte.');
  saveState({ sync: false });
  render();
  await writeJson('agentSync', { cmd: 'status' });
  scheduleSync(true);
}

function onBleDisconnected() {
  stopPhoneGps();
  chars = {};
  state.device.connected = false;
  log('BLE deconnecte.');
  saveState({ sync: false });
  render();
}

async function writeJson(name, payload) {
  const char = chars[name];
  if (!char) throw new Error(`BLE ${name} indisponible`);
  await char.writeValueWithResponse(enc(payload));
}

async function subscribe(name, callback) {
  const char = chars[name];
  if (!char) return;
  await char.startNotifications();
  char.addEventListener('characteristicvaluechanged', event => {
    try { callback(dec(event.target.value)); }
    catch (error) { log(`${name}: ${error.message}`); }
  });
}

function scheduleSync(now = false) {
  pendingSync = true;
  if (!state.device.connected) return;
  if (syncTimer) clearTimeout(syncTimer);
  syncTimer = setTimeout(() => syncToEsp32().catch(error => log(`Sync: ${error.message}`)), now ? 50 : 1200);
}

async function syncToEsp32() {
  if (!pendingSync || !state.device.connected) return;
  pendingSync = false;
  if (state.config.wifi.ssid) {
    await writeJson('wifiProvision', { ssid: state.config.wifi.ssid, pass: state.config.wifi.pass });
  }
  for (const [key, value] of Object.entries(state.config.keys)) {
    await writeJson('agentSync', { cmd: 'set_api_key', key, value: value || '' });
    await wait(20);
  }
  const systemEntries = [
    ['ble_config', 'device_name', state.config.system.bleName],
    ['system', 'timezone', state.config.system.timezone],
    ['compagnon', 'wake_word', state.config.system.wakeWord],
    ['compagnon', 'volume', String(state.config.system.volume)]
  ];
  for (const [ns, key, value] of systemEntries) {
    await writeJson('agentSync', { cmd: 'set_config', ns, key, value: value || '' });
    await wait(20);
  }
  await writeJson('agentSync', { cmd: 'set_config', ns: 'agents', key: 'json', value: JSON.stringify(state.agents) });
  await writeJson('agentSync', { cmd: 'set_config', ns: 'reminders', key: 'json', value: JSON.stringify(state.reminders) });
  await writeJson('agentSync', { cmd: 'set_config', ns: 'apps', key: 'json', value: JSON.stringify(state.config.apps) });
  state.device.lastSync = new Date().toISOString();
  log('Sync automatique ESP32 terminee.');
  saveState({ sync: false });
}

function wait(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

function renderNestor(root) {
  const grid = document.createElement('div');
  grid.className = 'grid';
  grid.append(appHero('Nestor', 'Conversation, routage des agents et actions CompagnonV2.'));

  const chatPanel = panel(`Chat - ${activeAgent().name}`, {
    className: 'two-third',
    actions: actions(agentSelect())
  });
  const chat = document.createElement('div');
  chat.className = 'chat';
  const messages = document.createElement('div');
  messages.className = 'messages';
  state.chat.filter(m => m.agentId === activeAgent().id).slice(-40).forEach(m => {
    const bubble = document.createElement('div');
    bubble.className = `message ${m.role === 'user' ? 'user' : ''}`;
    bubble.textContent = m.content;
    messages.append(bubble);
  });
  if (!messages.children.length) {
    const empty = document.createElement('div');
    empty.className = 'card muted';
    empty.textContent = 'Aucun message pour cet agent.';
    messages.append(empty);
  }
  const form = document.createElement('form');
  form.className = 'chat-form';
  const input = document.createElement('textarea');
  input.placeholder = 'Message pour Nestor...';
  const send = button('Envoyer', () => {}, 'primary');
  form.append(input, send);
  form.addEventListener('submit', async event => {
    event.preventDefault();
    const text = input.value.trim();
    if (!text) return;
    input.value = '';
    await sendNestorMessage(text);
  });
  chat.append(messages, form);
  chatPanel.append(chat);

  const statusPanel = panel('Etat', { className: 'third' });
  const status = document.createElement('div');
  status.className = 'list';
  status.append(
    metric('Agent actif', activeAgent().name),
    metric('Messages', String(state.chat.filter(m => m.agentId === activeAgent().id).length)),
    metric('Mode', state.config.keys.groq_key ? 'En ligne' : 'Offline', state.config.keys.groq_key ? 'ok' : 'warn'),
    metric('Derniere sync', state.device.lastSync ? new Date(state.device.lastSync).toLocaleTimeString() : '-')
  );
  statusPanel.append(status);

  grid.append(chatPanel, statusPanel);
  root.append(grid);
}

function agentSelect() {
  const select = document.createElement('select');
  select.style.width = '180px';
  state.agents.forEach(agent => {
    const opt = document.createElement('option');
    opt.value = agent.id;
    opt.textContent = agent.name;
    select.append(opt);
  });
  select.value = state.activeAgentId;
  select.addEventListener('change', () => {
    state.activeAgentId = select.value;
    saveState({ sync: false });
    render();
  });
  return select;
}

async function sendNestorMessage(text) {
  const agent = activeAgent();
  state.chat.push({ agentId: agent.id, role: 'user', content: text, ts: Date.now() });
  saveState();
  render();
  let reply = '';
  try {
    reply = await callGroq(agent, text);
  } catch (error) {
    reply = `Mode local/offline: message note pour ${agent.name}.\n\n${error.message}`;
  }
  state.chat.push({ agentId: agent.id, role: 'assistant', content: reply, ts: Date.now() });
  saveState();
  render();
}

async function callGroq(agent, text) {
  const key = state.config.keys.groq_key;
  if (!key) throw new Error('Cle Groq absente. Configure-la dans Parametres > APIs.');
  const history = state.chat.filter(m => m.agentId === agent.id).slice(-12).map(m => ({ role: m.role, content: m.content }));
  const res = await fetch('https://api.groq.com/openai/v1/chat/completions', {
    method: 'POST',
    headers: { 'content-type': 'application/json', authorization: `Bearer ${key}` },
    body: JSON.stringify({
      model: agent.model || 'llama-3.1-8b-instant',
      messages: [{ role: 'system', content: agent.prompt || '' }, ...history, { role: 'user', content: text }]
    })
  });
  if (!res.ok) throw new Error(`Groq HTTP ${res.status}`);
  const data = await res.json();
  return data.choices?.[0]?.message?.content || 'Reponse vide.';
}

function renderRappels(root) {
  const grid = document.createElement('div');
  grid.className = 'grid';
  grid.append(appHero('Rappels', 'Suivi des rappels et actions rapides.', actions(
    button('+ Rappel', addReminder, 'primary')
  )));
  const p = panel('Liste', { className: 'full' });
  const list = document.createElement('div');
  list.className = 'list';
  state.reminders.forEach(reminder => {
    const row = document.createElement('div');
    row.className = 'row';
    row.innerHTML = `<div><strong>${escapeHtml(reminder.label)}</strong><br><small class="muted">${escapeHtml(reminder.at)}${reminder.done ? ' - termine' : ''}</small></div>`;
    row.append(actions(
      button(reminder.done ? 'Reouvrir' : 'Terminer', () => { reminder.done = !reminder.done; saveState(); render(); }, 'ghost'),
      button('Supprimer', () => { state.reminders = state.reminders.filter(r => r.id !== reminder.id); saveState(); render(); }, 'danger')
    ));
    row.addEventListener('dblclick', () => editReminder(reminder));
    list.append(row);
  });
  if (!state.reminders.length) {
    const empty = document.createElement('div');
    empty.className = 'card muted';
    empty.textContent = 'Aucun rappel.';
    list.append(empty);
  }
  p.append(list);
  grid.append(p);
  root.append(grid);
}

function addReminder() {
  state.reminders.push({ id: `rem-${Date.now()}`, label: 'Nouveau rappel', at: new Date(Date.now() + 3600000).toISOString().slice(0, 16), done: false });
  saveState();
  render();
}

function editReminder(reminder) {
  const label = prompt('Rappel', reminder.label);
  if (label != null) reminder.label = label;
  const at = prompt('Date YYYY-MM-DDTHH:mm', reminder.at);
  if (at != null) reminder.at = at;
  saveState();
  render();
}

function renderMeteo(root) {
  const grid = document.createElement('div');
  grid.className = 'grid';
  grid.append(appHero('Meteo', 'Previsions, position et conditions utiles.', actions(
    button('Actualiser', refreshMeteo, 'primary'),
    button('Utiliser GPS', locateMeteo, 'ghost')
  )));
  const p = panel('Previsions', { className: 'full' });
  const cards = document.createElement('div');
  cards.className = 'card-grid';
  cards.append(
    metric('Ville', state.meteo.city || '-'),
    metric('Position', `${Number(state.meteo.lat).toFixed(4)}, ${Number(state.meteo.lon).toFixed(4)}`),
    metric('MAJ', state.meteo.updatedAt ? new Date(state.meteo.updatedAt).toLocaleString() : '-')
  );
  p.append(cards);
  const list = document.createElement('div');
  list.className = 'list';
  state.meteo.forecast.forEach(day => {
    const row = document.createElement('div');
    row.className = 'row';
    row.innerHTML = `<div><strong>${escapeHtml(day.day)}</strong><br><small class="muted">${escapeHtml(day.label)}</small></div><strong>${day.tmin} / ${day.tmax} deg</strong>`;
    list.append(row);
  });
  if (!state.meteo.forecast.length) {
    const empty = document.createElement('div');
    empty.className = 'card muted';
    empty.textContent = 'Aucune prevision chargee.';
    list.append(empty);
  }
  p.append(list);
  grid.append(p);
  root.append(grid);
}

function locateMeteo() {
  navigator.geolocation?.getCurrentPosition(pos => {
    state.meteo.lat = pos.coords.latitude;
    state.meteo.lon = pos.coords.longitude;
    saveState();
    refreshMeteo();
  }, err => log(`GPS meteo: ${err.message}`));
}

async function refreshMeteo() {
  try {
    const key = state.config.keys.meteo_key;
    if (!key) throw new Error('Cle Meteo-Concept absente.');
    const url = `https://api.meteo-concept.com/api/forecast/daily?token=${encodeURIComponent(key)}&latlng=${state.meteo.lat},${state.meteo.lon}`;
    const res = await fetch(url);
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const data = await res.json();
    state.meteo.city = data.city?.name || state.meteo.city;
    state.meteo.forecast = (data.forecast || []).slice(0, 7).map((d, i) => ({
      day: i === 0 ? 'Aujourd hui' : `J+${i}`,
      tmin: d.tmin,
      tmax: d.tmax,
      label: weatherLabel(d.weather)
    }));
    state.meteo.updatedAt = new Date().toISOString();
    saveState();
    render();
  } catch (error) {
    log(`Meteo: ${error.message}`);
  }
}

function weatherLabel(code = 0) {
  if (code <= 2) return 'Ensoleille';
  if (code <= 10) return 'Peu nuageux';
  if (code <= 16) return 'Couvert';
  if (code <= 22) return 'Pluie';
  if (code <= 28) return 'Orage';
  if (code <= 34) return 'Neige';
  return 'Variable';
}

function renderBourse(root) {
  const grid = document.createElement('div');
  grid.className = 'grid';
  grid.append(appHero('Bourse', 'Watchlist, prix et actifs suivis.', actions(
    button('Actualiser', refreshBourse, 'primary')
  )));
  const p = panel('Watchlist', { className: 'full' });
  const list = document.createElement('div');
  list.className = 'list';
  state.bourse.quotes.forEach(q => {
    const row = document.createElement('div');
    row.className = 'row';
    row.innerHTML = `<div><strong>${escapeHtml(q.symbol)}</strong><br><small class="muted">${escapeHtml(q.name || '')}</small></div><strong>${q.price || '-'} ${q.currency || ''}</strong>`;
    list.append(row);
  });
  if (!state.bourse.quotes.length) {
    const card = document.createElement('div');
    card.className = 'card muted';
    card.textContent = `Tickers configures: ${state.bourse.tickers.join(', ')}`;
    list.append(card);
  }
  p.append(list);
  grid.append(p);
  root.append(grid);
}

async function refreshBourse() {
  try {
    const key = state.config.keys.twdata_key;
    if (!key) throw new Error('Cle Twelve Data absente.');
    const symbols = state.bourse.tickers.join(',');
    const url = `https://api.twelvedata.com/quote?symbol=${encodeURIComponent(symbols)}&apikey=${encodeURIComponent(key)}`;
    const res = await fetch(url);
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const data = await res.json();
    const values = data.symbol ? [data] : Object.values(data);
    state.bourse.quotes = values.map(q => ({ symbol: q.symbol, name: q.name, price: q.close, currency: q.currency }));
    state.bourse.updatedAt = new Date().toISOString();
    saveState();
    render();
  } catch (error) {
    log(`Bourse: ${error.message}`);
  }
}

function renderDomotique(root) {
  const grid = document.createElement('div');
  grid.className = 'grid';
  grid.append(appHero('Domotique', 'Pilotage maison et scenes rapides.', actions(
    button('Commande', () => sendTextIntent('domotique ' + (prompt('Commande maison') || '')), 'primary')
  )));
  const p = panel('Maison', { className: 'full' });
  const controls = document.createElement('div');
  controls.className = 'control-grid';
  [
    ['Lumiere salon', 'domotique lumiere salon toggle'],
    ['Mode nuit', 'domotique scene nuit'],
    ['Tout eteindre', 'domotique tout eteindre'],
    ['Presence', 'domotique presence status']
  ].forEach(([label, intent]) => controls.append(button(label, () => sendTextIntent(intent), label === 'Tout eteindre' ? 'danger' : '')));
  const status = document.createElement('div');
  status.className = 'card-grid';
  status.append(
    metric('Connexion', state.device.connected ? 'ESP32 pret' : 'Offline', state.device.connected ? 'ok' : 'warn'),
    metric('Derniere action', state.logs[0] || '-')
  );
  p.append(controls, status);
  grid.append(p);
  root.append(grid);
}

function renderEcovacs(root) {
  const grid = document.createElement('div');
  grid.className = 'grid';
  grid.append(appHero('Ecovacs', 'Pilotage du robot et actions de nettoyage.', actions(
    button('Start', () => sendTextIntent('ecovacs start'), 'primary'),
    button('Retour base', () => sendTextIntent('ecovacs dock'), 'ghost')
  )));
  const p = panel('Robot', { className: 'full' });
  const controls = document.createElement('div');
  controls.className = 'control-grid';
  [
    ['Start', 'ecovacs start'],
    ['Pause', 'ecovacs pause'],
    ['Stop', 'ecovacs stop'],
    ['Retour base', 'ecovacs dock'],
    ['Auto', 'ecovacs clean auto'],
    ['Spot', 'ecovacs clean spot']
  ].forEach(([label, intent]) => controls.append(button(label, () => sendTextIntent(intent), label === 'Start' ? 'primary' : '')));
  const status = document.createElement('div');
  status.className = 'card-grid';
  status.append(
    metric('Etat', state.device.connected ? 'Pret via ESP32' : 'Offline', state.device.connected ? 'ok' : 'warn'),
    metric('Robot', state.config.keys.ecovacs_dev || 'Auto')
  );
  p.append(controls, status);
  grid.append(p);
  root.append(grid);
}

async function sendTextIntent(text) {
  try {
    if (!text.trim()) return;
    await writeJson('textInput', { text });
    log(`Commande envoyee: ${text}`);
  } catch (error) {
    log(`Commande BLE: ${error.message}`);
  }
}

function renderRadars(root) {
  const grid = document.createElement('div');
  grid.className = 'grid';
  grid.append(appHero('Radars', 'Vue conduite, vitesse et alertes radar.', actions(
    button(state.radar.running ? 'Stop GPS' : 'Start GPS', toggleRadar, 'primary'),
    button('Charger radars', refreshRadarData, 'ghost')
  )));
  const p = panel('Radar Alert', { className: 'full' });
  const screen = document.createElement('div');
  screen.className = 'radar-screen';
  const ring = document.createElement('div');
  ring.className = `speed-ring ${state.radar.nearest?.dist <= getAlertZone(state.radar.nearest?.speed || 50).before ? 'alert' : ''}`;
  ring.innerHTML = `<div><div class="speed-value">${Math.round(state.radar.speed || 0)}</div><div class="speed-unit">km/h</div></div>`;
  const cards = document.createElement('div');
  cards.className = 'card-grid';
  cards.append(
    metric('GPS', state.radar.pos ? `${state.radar.pos.lat.toFixed(4)}, ${state.radar.pos.lon.toFixed(4)}` : '-'),
    metric('Radars cache', String(state.radar.radars.length)),
    metric('Plus proche', state.radar.nearest ? `${Math.round(state.radar.nearest.dist)} m` : '-'),
    metric('Limite', state.radar.nearest?.speed ? `${state.radar.nearest.speed} km/h` : '-')
  );
  screen.append(ring, cards);
  p.append(screen);
  grid.append(p);
  root.append(grid);
}

function renderSettings(root) {
  const shell = document.createElement('div');
  shell.className = 'settings-shell';
  const tabs = document.createElement('nav');
  tabs.className = 'settings-tabs';
  SETTINGS_TABS.forEach(([id, label]) => {
    const tab = button(label, () => setSettingsTab(id));
    tab.classList.toggle('active', state.settingsTab === id);
    tabs.append(tab);
  });
  const content = document.createElement('div');
  content.className = 'settings-content';
  const renderers = {
    global: settingsGlobal,
    esp32: settingsEsp32,
    nestor: settingsNestor,
    rappels: settingsRappels,
    meteo: settingsMeteo,
    bourse: settingsBourse,
    radars: settingsRadars,
    domotique: settingsDomotique,
    ecovacs: settingsEcovacs,
    apis: settingsApis,
    sync: settingsSync
  };
  (renderers[state.settingsTab] || settingsGlobal)(content);
  shell.append(tabs, content);
  root.append(shell);
}

function settingsPanel(title, subtitle) {
  const grid = document.createElement('div');
  grid.className = 'grid';
  grid.append(panel(title, { className: 'full', subtitle }));
  return grid;
}

function appendToFirstPanel(grid, ...nodes) {
  grid.querySelector('.panel').append(...nodes);
}

function settingsGlobal(root) {
  const grid = settingsPanel('Global', 'Preferences generales de la PWA et activation des apps.');
  const form = document.createElement('div');
  form.className = 'form-grid';
  form.append(
    field('Theme', state.theme, v => { state.theme = v; applyTheme(); }, { select: [['auto', 'Auto systeme'], ['light', 'Clair'], ['dark', 'Sombre']] }),
    field('Auto-sync BLE', state.config.system.autoSync, v => state.config.system.autoSync = v, { type: 'checkbox', inline: true }),
    field('Wake word', state.config.system.wakeWord, v => state.config.system.wakeWord = v),
    field('Volume', state.config.system.volume, v => state.config.system.volume = Number(v), { type: 'number' })
  );
  appendToFirstPanel(grid, form, appToggles());
  root.append(grid);
}

function appToggles() {
  const wrap = document.createElement('div');
  wrap.className = 'form-grid';
  Object.keys(state.config.apps).forEach(app => {
    wrap.append(field(`App ${app}`, state.config.apps[app], v => state.config.apps[app] = v, { type: 'checkbox', inline: true }));
  });
  return wrap;
}

function settingsEsp32(root) {
  const grid = settingsPanel('ESP32', 'Connexion BLE, WiFi, NVS et relais GPS telephone vers CompagnonV2.');
  const stats = document.createElement('div');
  stats.className = 'card-grid';
  stats.append(
    metric('Device', state.device.name || '-'),
    metric('BLE', state.device.connected ? 'connecte' : 'deconnecte', state.device.connected ? 'ok' : 'warn'),
    metric('WiFi ESP32', state.device.status.wifi || '-'),
    metric('IP', state.device.status.ip || '-')
  );
  const form = document.createElement('div');
  form.className = 'form-grid';
  form.append(
    field('SSID', state.config.wifi.ssid, v => state.config.wifi.ssid = v),
    field('Mot de passe WiFi', state.config.wifi.pass, v => state.config.wifi.pass = v, { type: 'password' }),
    field('Nom BLE', state.config.system.bleName, v => state.config.system.bleName = v),
    field('Timezone', state.config.system.timezone, v => state.config.system.timezone = v),
    field('Relais GPS telephone', state.config.system.phoneGpsRelay, v => { state.config.system.phoneGpsRelay = v; v ? startPhoneGps() : stopPhoneGps(); }, { type: 'checkbox', inline: true })
  );
  appendToFirstPanel(grid, stats, form, actions(
    button('Scan WiFi', scanWifi, 'ghost'),
    button('Push NVS', () => scheduleSync(true), 'primary'),
    button(state.config.system.phoneGpsRelay ? 'Stop GPS BLE' : 'Start GPS BLE', () => {
      state.config.system.phoneGpsRelay = !state.config.system.phoneGpsRelay;
      state.config.system.phoneGpsRelay ? startPhoneGps() : stopPhoneGps();
      saveState();
      render();
    }, 'ghost')
  ), wifiResultsList());
  root.append(grid);
}

function wifiResultsList() {
  const list = document.createElement('div');
  list.className = 'list';
  state.device.wifiResults.forEach(net => {
    const row = document.createElement('div');
    row.className = 'row';
    row.innerHTML = `<div><strong>${escapeHtml(net.ssid)}</strong><br><small class="muted">${net.rssi} dBm</small></div>`;
    row.append(button('Choisir', () => {
      state.config.wifi.ssid = net.ssid;
      saveState();
      render();
    }, 'ghost'));
    list.append(row);
  });
  return list;
}

function settingsNestor(root) {
  const grid = settingsPanel('Nestor & Agents', 'Creation, edition et synchronisation des agents. Le chat reste dans l app Nestor.');
  const agent = activeAgent();
  const form = document.createElement('div');
  form.className = 'form-grid';
  form.append(
    field('Nom', agent.name, v => { agent.name = v; agent.updatedAt = new Date().toISOString(); }),
    field('Role', agent.role, v => { agent.role = v; agent.updatedAt = new Date().toISOString(); }),
    field('Modele', agent.model, v => { agent.model = v; agent.updatedAt = new Date().toISOString(); }),
    field('Prompt systeme', agent.prompt, v => { agent.prompt = v; agent.updatedAt = new Date().toISOString(); }, { textarea: true })
  );
  appendToFirstPanel(grid, actions(
    agentSelect(),
    button('+ Agent', addAgent, 'primary'),
    button('Supprimer', deleteActiveAgent, 'danger')
  ), form);
  root.append(grid);
}

function addAgent() {
  const id = `agent-${Date.now()}`;
  state.agents.push({ id, name: 'Nouvel agent', role: 'generic', model: 'llama-3.1-8b-instant', prompt: '', updatedAt: new Date().toISOString() });
  state.activeAgentId = id;
  saveState();
  render();
}

function deleteActiveAgent() {
  if (state.agents.length <= 1) return;
  state.agents = state.agents.filter(a => a.id !== state.activeAgentId);
  state.activeAgentId = state.agents[0].id;
  saveState();
  render();
}

function settingsRappels(root) {
  const grid = settingsPanel('Rappels', 'Options de synchro et comportement des rappels.');
  appendToFirstPanel(grid, field('Synchroniser les rappels vers ESP32', true, () => {}, { type: 'checkbox', inline: true }));
  root.append(grid);
}

function settingsMeteo(root) {
  const grid = settingsPanel('Meteo', 'Cle API et position de reference utilisees par la vue meteo.');
  const form = document.createElement('div');
  form.className = 'form-grid';
  form.append(
    field('Cle Meteo-Concept', state.config.keys.meteo_key, v => state.config.keys.meteo_key = v, { type: 'password' }),
    field('Ville', state.meteo.city, v => state.meteo.city = v),
    field('Latitude', state.meteo.lat, v => state.meteo.lat = Number(v), { type: 'number' }),
    field('Longitude', state.meteo.lon, v => state.meteo.lon = Number(v), { type: 'number' })
  );
  appendToFirstPanel(grid, form);
  root.append(grid);
}

function settingsBourse(root) {
  const grid = settingsPanel('Bourse', 'Cle Twelve Data et watchlist.');
  const form = document.createElement('div');
  form.className = 'form-grid';
  form.append(
    field('Cle Twelve Data', state.config.keys.twdata_key, v => state.config.keys.twdata_key = v, { type: 'password' }),
    field('Tickers', state.bourse.tickers.join(', '), v => state.bourse.tickers = v.split(',').map(x => x.trim()).filter(Boolean))
  );
  appendToFirstPanel(grid, form);
  root.append(grid);
}

function settingsRadars(root) {
  const grid = settingsPanel('Radars', 'Source radar, proxy et rayon de recherche. La vue conduite reste dans l app Radars.');
  const form = document.createElement('div');
  form.className = 'form-grid';
  form.append(
    field('Source', state.radar.source, v => state.radar.source = v, { select: [['lufop', 'Lufop'], ['blitzer', 'Blitzer']] }),
    field('Proxy CORS optionnel', state.radar.proxy, v => state.radar.proxy = v),
    field('Max radars', state.radar.maxRadars, v => state.radar.maxRadars = Number(v), { type: 'number' }),
    field('Rayon km', state.radar.radiusKm, v => state.radar.radiusKm = Number(v), { type: 'number' }),
    field('Zoom Blitzer', state.radar.zoom, v => state.radar.zoom = Number(v), { type: 'number' })
  );
  appendToFirstPanel(grid, form);
  root.append(grid);
}

function settingsDomotique(root) {
  const grid = settingsPanel('Domotique', 'Identifiants Tuya/SmartLife. Les commandes sont dans l app Domotique.');
  const form = document.createElement('div');
  form.className = 'form-grid';
  form.append(
    field('Tuya Access ID', state.config.keys.tuya_id, v => state.config.keys.tuya_id = v, { type: 'password' }),
    field('Tuya Secret', state.config.keys.tuya_sec, v => state.config.keys.tuya_sec = v, { type: 'password' }),
    field('Region', state.config.keys.tuya_region, v => state.config.keys.tuya_region = v),
    field('User ID', state.config.keys.tuya_user, v => state.config.keys.tuya_user = v, { type: 'password' })
  );
  appendToFirstPanel(grid, form);
  root.append(grid);
}

function settingsEcovacs(root) {
  const grid = settingsPanel('Ecovacs', 'Compte Ecovacs et robot cible. Les commandes sont dans l app Ecovacs.');
  const form = document.createElement('div');
  form.className = 'form-grid';
  form.append(
    field('Email', state.config.keys.ecovacs_u, v => state.config.keys.ecovacs_u = v),
    field('Mot de passe', state.config.keys.ecovacs_p, v => state.config.keys.ecovacs_p = v, { type: 'password' }),
    field('Pays', state.config.keys.ecovacs_cc, v => state.config.keys.ecovacs_cc = v),
    field('Device ID', state.config.keys.ecovacs_dev, v => state.config.keys.ecovacs_dev = v)
  );
  appendToFirstPanel(grid, form);
  root.append(grid);
}

function settingsApis(root) {
  const grid = settingsPanel('APIs', 'Cles transverses utilisees par Nestor et les apps.');
  const form = document.createElement('div');
  form.className = 'form-grid';
  const labels = {
    groq_key: 'Groq',
    gemini_key: 'Gemini',
    serper_key: 'Serper',
    openrtr_key: 'OpenRouter',
    spotify_id: 'Spotify ID',
    spotify_sec: 'Spotify Secret'
  };
  Object.entries(labels).forEach(([key, label]) => form.append(field(label, state.config.keys[key], v => state.config.keys[key] = v, { type: 'password' })));
  appendToFirstPanel(grid, form);
  root.append(grid);
}

function settingsSync(root) {
  const grid = settingsPanel('Sync', 'Journal local, export et synchronisation manuelle.');
  const stats = document.createElement('div');
  stats.className = 'card-grid';
  stats.append(
    metric('Derniere sync', state.device.lastSync ? new Date(state.device.lastSync).toLocaleString() : '-'),
    metric('Agents', String(state.agents.length)),
    metric('Rappels', String(state.reminders.length)),
    metric('BLE', state.device.connected ? 'connecte' : 'deconnecte', state.device.connected ? 'ok' : 'warn')
  );
  const pre = document.createElement('pre');
  pre.className = 'log';
  pre.textContent = state.logs.join('\n');
  appendToFirstPanel(grid, stats, actions(
    button('Sync maintenant', () => scheduleSync(true), 'primary'),
    button('Exporter JSON', exportJson, 'ghost')
  ), pre);
  root.append(grid);
}

async function scanWifi() {
  try {
    await writeJson('wifiScan', { cmd: 'scan' });
    log('Scan WiFi demande.');
  } catch (error) {
    log(`Scan WiFi: ${error.message}`);
  }
}

function exportJson() {
  navigator.clipboard?.writeText(JSON.stringify(state, null, 2));
  log('Etat PWA copie dans le presse-papiers.');
  render();
}

function toggleRadar() {
  if (state.radar.running) stopRadar();
  else startRadar();
  render();
}

function startRadar() {
  if (!navigator.geolocation) {
    log('GPS navigateur indisponible.');
    return;
  }
  state.radar.running = true;
  radarWatch = navigator.geolocation.watchPosition(async pos => {
    const prev = state.radar.pos;
    state.radar.pos = { lat: pos.coords.latitude, lon: pos.coords.longitude };
    state.radar.speed = pos.coords.speed ? pos.coords.speed * 3.6 : 0;
    state.radar.heading = pos.coords.heading || state.radar.heading || 0;
    if (!state.radar.radars.length || !prev || haversine(prev.lat, prev.lon, state.radar.pos.lat, state.radar.pos.lon) > 5000) {
      refreshRadarData().catch(error => log(`Radars: ${error.message}`));
    }
    computeRadarAlert();
    saveState();
    render();
  }, error => log(`GPS radar: ${error.message}`), { enableHighAccuracy: true, maximumAge: 0, timeout: 5000 });
  log('Radar GPS actif.');
  saveState();
}

function stopRadar() {
  if (radarWatch != null) navigator.geolocation.clearWatch(radarWatch);
  radarWatch = null;
  state.radar.running = false;
  saveState();
}

async function refreshRadarData() {
  if (!state.radar.pos) {
    log('Radars: GPS requis avant chargement.');
    return;
  }
  const data = state.radar.source === 'blitzer' ? await loadBlitzer() : await loadLufop();
  state.radar.radars = data.slice(0, state.radar.maxRadars);
  state.radar.updatedAt = new Date().toISOString();
  computeRadarAlert();
  log(`${state.radar.radars.length} radars charges (${state.radar.source}).`);
  saveState();
  render();
}

async function loadLufop() {
  const url = `https://api.lufop.net/api?format=json&nbr=${state.radar.maxRadars}`;
  const data = await fetchJson(withProxy(url));
  return (Array.isArray(data) ? data : data.radars || data.features || []).map(x => ({
    lat: Number(x.lat ?? x.latitude ?? x.geometry?.coordinates?.[1]),
    lon: Number(x.lon ?? x.lng ?? x.longitude ?? x.geometry?.coordinates?.[0]),
    speed: parseInt(x.vitesse ?? x.speed ?? x.properties?.speed ?? 50, 10),
    azimut: parseFloat(x.azimut ?? x.heading ?? 0),
    flash: x.flash ?? '',
    position: x.position ?? ''
  })).filter(r => Number.isFinite(r.lat) && Number.isFinite(r.lon));
}

async function loadBlitzer() {
  const p = state.radar.pos;
  const deg = Number(state.radar.radiusKm || 50) / 111;
  const box = `${p.lat - deg},${p.lon - deg},${p.lat + deg},${p.lon + deg}`;
  const url = `https://cdn2.atudo.net/api/4.0/pois.php?z=${state.radar.zoom}&type=0,1,2,3,4,5,ra,w&box=${encodeURIComponent(box)}`;
  const data = await fetchJson(withProxy(url));
  return (data.pois || data.spots || []).map(x => ({
    lat: Number(x.lat),
    lon: Number(x.lng ?? x.lon),
    speed: parseInt(x.speed ?? x.vmax ?? 50, 10),
    azimut: parseFloat(x.angle ?? 0),
    flash: x.flash ?? 'D',
    position: ''
  })).filter(r => Number.isFinite(r.lat) && Number.isFinite(r.lon));
}

function withProxy(url) {
  return state.radar.proxy ? `${state.radar.proxy.replace(/\/$/, '')}?url=${encodeURIComponent(url)}` : url;
}

async function fetchJson(url) {
  const res = await fetch(url);
  if (!res.ok) throw new Error(`HTTP ${res.status}`);
  return res.json();
}

function computeRadarAlert() {
  const p = state.radar.pos;
  if (!p || !state.radar.radars.length) {
    state.radar.nearest = null;
    return;
  }
  const relevant = state.radar.radars.map(r => {
    const dist = haversine(p.lat, p.lon, r.lat, r.lon);
    const toRadar = bearing(p.lat, p.lon, r.lat, r.lon);
    const detectionAngle = getDetectionAngle(state.radar.speed);
    const bearingDiff = angleDiff(state.radar.heading, toRadar);
    const headingDiff = angleDiff(state.radar.heading, Number(r.azimut || 0));
    const zone = getAlertZone(r.speed || 50);
    const goingTowards = bearingDiff <= detectionAngle;
    const pointsAtMe = headingDiff <= 60;
    const bidirectional = r.flash === 'D' || !r.azimut;
    const relevant = r.position !== 'L' && dist <= zone.before && goingTowards && (bidirectional || pointsAtMe);
    return { ...r, dist, relevant };
  }).filter(r => r.relevant).sort((a, b) => a.dist - b.dist);
  state.radar.nearest = relevant[0] || state.radar.radars
    .map(r => ({ ...r, dist: haversine(p.lat, p.lon, r.lat, r.lon) }))
    .sort((a, b) => a.dist - b.dist)[0] || null;
  if (state.radar.nearest && state.radar.nearest.dist <= getAlertZone(state.radar.nearest.speed || 50).before) {
    radarBeep();
  }
}

function getAlertZone(speed) {
  if (speed >= 130) return { before: 1000, after: 300 };
  if (speed >= 110) return { before: 800, after: 200 };
  if (speed >= 90) return { before: 500, after: 150 };
  if (speed >= 70) return { before: 350, after: 100 };
  return { before: 250, after: 100 };
}

function getDetectionAngle(speed) {
  if (speed <= 50) return 43;
  if (speed <= 70) return 18;
  if (speed <= 90) return 13;
  if (speed <= 110) return 9;
  if (speed <= 130) return 6;
  return 5;
}

function haversine(lat1, lon1, lat2, lon2) {
  const R = 6371e3;
  const p1 = lat1 * Math.PI / 180;
  const p2 = lat2 * Math.PI / 180;
  const dlat = (lat2 - lat1) * Math.PI / 180;
  const dlon = (lon2 - lon1) * Math.PI / 180;
  const a = Math.sin(dlat / 2) ** 2 + Math.cos(p1) * Math.cos(p2) * Math.sin(dlon / 2) ** 2;
  return R * 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
}

function bearing(lat1, lon1, lat2, lon2) {
  const dLon = (lon2 - lon1) * Math.PI / 180;
  const y = Math.sin(dLon) * Math.cos(lat2 * Math.PI / 180);
  const x = Math.cos(lat1 * Math.PI / 180) * Math.sin(lat2 * Math.PI / 180) -
    Math.sin(lat1 * Math.PI / 180) * Math.cos(lat2 * Math.PI / 180) * Math.cos(dLon);
  return ((Math.atan2(y, x) * 180 / Math.PI) + 360) % 360;
}

function angleDiff(a, b) {
  return Math.abs(((a - b + 540) % 360) - 180);
}

function radarBeep() {
  try {
    radarAudio ||= new (window.AudioContext || window.webkitAudioContext)();
    const osc = radarAudio.createOscillator();
    const gain = radarAudio.createGain();
    osc.connect(gain);
    gain.connect(radarAudio.destination);
    osc.frequency.value = state.radar.speed > (state.radar.nearest?.speed || 999) ? 1800 : 900;
    gain.gain.setValueAtTime(0.12, radarAudio.currentTime);
    gain.gain.exponentialRampToValueAtTime(0.001, radarAudio.currentTime + 0.18);
    osc.start();
    osc.stop(radarAudio.currentTime + 0.18);
  } catch {}
}

function startPhoneGps() {
  if (phoneGpsWatch != null || !navigator.geolocation) return;
  phoneGpsWatch = navigator.geolocation.watchPosition(async pos => {
    const payload = {
      lat: pos.coords.latitude,
      lon: pos.coords.longitude,
      alt: pos.coords.altitude || 0,
      speed: pos.coords.speed ? pos.coords.speed * 3.6 : 0,
      accuracy: pos.coords.accuracy,
      ts: Date.now()
    };
    try { await writeJson('gps', payload); }
    catch (error) { log(`GPS BLE: ${error.message}`); }
  }, error => log(`GPS: ${error.message}`), { enableHighAccuracy: true, maximumAge: 0, timeout: 5000 });
}

function stopPhoneGps() {
  if (phoneGpsWatch != null) navigator.geolocation.clearWatch(phoneGpsWatch);
  phoneGpsWatch = null;
}

function escapeHtml(value) {
  return String(value ?? '').replace(/[&<>"']/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#039;' }[c]));
}

function bind() {
  $$('.nav-section button').forEach(button => button.addEventListener('click', () => setView(button.dataset.view)));
  $('#connectBle').addEventListener('click', () => connectBle().catch(error => { log(error.message); render(); }));
  $('#themeSelect').addEventListener('change', event => {
    state.theme = event.target.value;
    saveState({ sync: false });
    applyTheme();
  });
}

if ('serviceWorker' in navigator) {
  navigator.serviceWorker.register('./service-worker.js').catch(error => log(`SW: ${error.message}`));
}

bind();
render();
log('PWA CompagnonV2 prete.');
