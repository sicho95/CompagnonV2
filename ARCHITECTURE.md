# CompagnonV2 — Architecture détaillée

> Document complémentaire à SPEC.md — détaille les choix d'implémentation par module.

---

## 1. Firmware ESP32-S3

### 1.1 Toolchain

- **PlatformIO** recommandé (Arduino framework sur ESP32-S3), même logique que Compagnon v1.
- Framework : `arduino` ou `espidf` selon les besoins (arduino pour compatibilité lib, espidf pour ULP/light sleep avancé).
- Cible board : `esp32-s3-devkitc-1` ou config custom Waveshare AMOLED 2.16".

### 1.2 Partition table (16MB flash)

```csv
# Name,   Type, SubType,  Offset,   Size,     Flags
nvs,      data, nvs,      0x9000,   0x5000,
otadata,  data, ota,      0xe000,   0x2000,
app0,     app,  ota_0,    0x10000,  0x300000,
app1,     app,  ota_1,    0x310000, 0x300000,
assets,   data, spiffs,   0x610000, 0x200000,
fatfs,    data, fat,      0x810000, 0x7F0000,
```

- `fatfs` : agents, mémoire L0-L4, rappels, sessions, skills, logs.
- `assets` : icônes LVGL, sons I2S, police fonts.
- SD card (si présente) : archives longues, debug logs.

### 1.3 Module `config/`

```
config/
  pins.h           # GPIO display, touch, PMU I2C, audio I2S, buttons, SD CS
  secrets.h        # (gitignored) clés API, SSID de dev, wake_word name
  secrets_template.h  # template versionné sans secrets
  lv_conf.h        # configuration LVGL (résolution, color depth...)
```

### 1.4 Module `hal/`

```
hal/
  display.h/.cpp   # init rm67162 (SPI/QSPI), rotation, backlight
  touch.h/.cpp     # init CST816S ou équivalent, events touch → LVGL input driver
  pmu.h/.cpp       # AXP2101 I2C : batterie %, tension, charge, PMIC config
  audio_io.h/.cpp  # init I2S mic + I2S DAC/amp, read/write buffers, DMA
  rtc.h/.cpp       # PCF85063 ou RTC interne ESP32 : get/set time, alarms
  sd_card.h/.cpp   # init SPI SD, mount/umount FAT, read/write
  imu.h/.cpp       # optionnel : QMI8658 ou MPU6050 si présent
```

### 1.5 Module `net/`

```
net/
  wifi_mgr.h/.cpp     # connect, reconnect, getSSID, scan, provisioning BLE
                      # portail AP en fallback si pas de creds
  ble_mgr.h/.cpp      # GATT server, service Compagnon, 8 characteristics
  ble_protocol.h/.cpp # parsing/sérialisation frames JSON BLE
  http_client.h/.cpp  # GET/POST JSON sur WiFi, gestion timeout/retry
                      # si NO WIFI : délègue via ble_relay_request()
  ota.h/.cpp          # OTA via WiFi (ESP-IDF OTA ou ArduinoOTA)
```

### 1.6 Module `system/`

```
system/
  os_main.h/.cpp        # task_os_main : tick de tous les sous-systèmes
  power_mgr.h/.cpp      # light sleep, wakeup sources, screen timeout
  scheduler.h/.cpp      # cron jobs (jardinier, refresh apps, rappels)
  time_mgr.h/.cpp       # NTP sync, RTC read/write, format date FR
  mode_mgr.h/.cpp       # gestion modes (normal, silent, low_battery...)
  app_orchestrator.h/.cpp  # activation/suspension apps, routing vocal → app
```

### 1.7 Module `voice/`

```
voice/
  wake_word.h/.cpp  # ESP-SR : init modèle, detection loop, callback
  asr.h/.cpp        # STT : capture audio → Groq Whisper HTTP ou BLE relay
  tts.h/.cpp        # TTS : texte → Groq PlayAI → I2S ou BLE relay Web Speech
```

### 1.8 Module `agent_brain/`

Basé sur l'architecture mimiclaw de Compagnon2 :

```
agent_brain/
  react_engine.h/.cpp   # boucle ReAct : plan → tool → observe
  agents_loader.h/.cpp  # charge agents depuis FATFS JSON
  tools/
    tool_base.h
    tool_http.h/.cpp     # appel HTTP JSON (GET/POST)
    tool_weather.h/.cpp  # météo OpenWeatherMap
    tool_quote.h/.cpp    # cours boursiers Yahoo Finance
    tool_reminder.h/.cpp # CRUD rappels
    tool_smarthome.h/.cpp
    tool_ecovacs.h/.cpp
  memory/
    memory_l0.h/.cpp     # contraintes système
    memory_l1.h/.cpp     # index skills
    memory_l2.h/.cpp     # MEMORY.md faits utilisateur
    memory_l3.h/.cpp     # skills auto-cristallisées
    memory_l4.h/.cpp     # sessions
  jardinier.h/.cpp       # consolidation mémoire, nettoyage skills
  fabrique.h/.cpp        # création agents à la volée
  crystallizer.h/.cpp    # détection workflows récurrents → skills L3
```

### 1.9 Module `storage/`

```
storage/
  nvs_mgr.h/.cpp       # wrappers NVS : get/set string/int/bool, namespace
  fatfs_mgr.h/.cpp     # mount/umount FATFS, mkdir, list, quota
  sd_mgr.h/.cpp        # mount/umount SD, cold archive read/write
  json_io.h/.cpp       # read/write/merge ArduinoJson dans FATFS/SD
  reminders_db.h/.cpp  # CRUD rappels : load, save, get_next_alarm
```

### 1.10 Module `apps/`

Chaque app : dossier dédié avec `app_base.h` implémenté.

```
apps/
  app_base.h            # interface : init, start, stop, tick, voice_input
  app_registry.h/.cpp   # liste des apps enregistrées, lookup par nom/id
  bourse/
    bourse_app.h/.cpp   # API Yahoo Finance/Alpha, watchlist, alertes
  meteo/
    meteo_app.h/.cpp    # API OpenWeatherMap, forecast, icônes
  nestor/
    nestor_app.h/.cpp   # UI chat LVGL + agent brain bridge
  musique/
    musique_app.h/.cpp  # streaming audio (Spotify/YT music via BLE relay ou WiFi)
  radars/
    radars_app.h/.cpp   # API radars (vol, radar météo, trafic...)
  smarthome/
    smarthome_app.h/.cpp  # Home Assistant/local MQTT
  ecovacs/
    ecovacs_app.h/.cpp    # Ecovacs API
  rappels/
    rappels_app.h/.cpp    # UI LVGL : liste + création + alarme
```

### 1.11 Module `ui/`

```
ui/
  launcher.h/.cpp      # tileview 8 apps, navigation, app_start/stop
  status_bar.h/.cpp    # date jj mmm aaaa - hh:mm, BLE, WiFi, batterie
  voice_overlay.h/.cpp # vue "listening" (waveform LVGL), feedback STT
  apps/                # UI LVGL spécifique par app (nestor_ui, rappels_ui...)
```

---

## 2. Compagnon PWA

### 2.1 Stack technique

- Vanilla JS (ES modules) — pas de framework, comme Compagnon v1.
- CSS custom (responsive, dark theme par défaut).
- Service Worker : cache offline, background sync.
- Web Bluetooth API (Chrome Android / Desktop / Bluefy iOS).

### 2.2 Architecture modules

```
pwa/src/
  app.js              # entry : init SW, routing hash, render shell
  core/
    orchestrator-engine.js  # ReAct loop côté PWA, agents, tâches cron
    tts.js                  # cascade TTS (Groq PlayAI, Web Speech)
    agent-loader.js         # charge agents depuis IndexedDB
    crystallizer.js         # cristallisation skills côté PWA
    scheduler.js            # cron jobs PWA (ServiceWorker BgSync)
  api/
    backends.js       # Groq, Gemini, Perplexity, OpenWeatherMap...
    weather.js
    bourse.js
    radars.js
    ecovacs.js
    smarthome.js
  bt/
    ble.js            # connect, disconnect, scan BLE devices
    ble_protocol.js   # sérialisation frames, LLM relay, chunks MTU
    ble_status.js     # état global BLE (connected, device_name, mode)
  device/
    provisioning.js   # scan WiFi BLE, provisionner mdp
    device_settings.js  # config ESP32 (apps actives, clés API, prefs)
    device_config.js    # load/save config locale (localStorage)
  input/
    bt_keyboard.js    # clavier overlay → TEXT_INPUT BLE
    voice_input.js    # Web Speech STT + mic button
  storage/
    idb.js            # wrappers IndexedDB (agents, mémoire, rappels, sessions)
    local.js          # wrappers localStorage (config, clés API)
  sync/
    agents_sync.js    # sync agents PWA ↔ ESP32 (AGENT_SYNC BLE)
    reminders_sync.js # sync rappels PWA ↔ ESP32 (REMINDERS_SYNC BLE)
    memory_sync.js    # sync mémoire L2 PWA ↔ ESP32
  ui/
    app-shell.js      # shell : header, bottom-nav, sidebar desktop
    launcher.js       # page accueil : cards 8 apps
    compagnon.js      # config ESP32 : BLE, WiFi, apps, clés API, sync
    bourse-view.js
    meteo-view.js
    nestor-view.js    # chat Nestor + bouton micro
    musique-view.js
    radar-view.js
    smarthome.js
    ecovacs-view.js
    rappels-view.js   # liste rappels + formulaire + input vocal
```

### 2.3 Responsive layout

```css
/* Cibles breakpoints */
/* S  : 375px  iPhone SE / 13 mini */
/* M  : 390px  iPhone 13/14/15 — référence principale */
/* L  : 768px  iPad / tablettes Android */
/* XL : 1024px Desktop */

.app-shell {
  display: grid;
  /* mobile : colonne unique + bottom-nav */
  grid-template-rows: 48px 1fr 56px;
  /* desktop : sidebar 220px + content */
}
@media (min-width: 768px) {
  .app-shell {
    grid-template-columns: 220px 1fr;
    grid-template-rows: 48px 1fr;
  }
  .bottom-nav { display: none; }
  .sidebar { display: flex; }
}
```

### 2.4 Standalone PWA (sans ESP32)

- Toutes les vues d'apps fonctionnent directement avec leurs APIs cloud.
- La vue "Compagnon" (config ESP32) reste accessible mais signale "ESP32 non connecté".
- Aucune dépendance bloquante au BLE dans le rendu des apps.

---

## 3. Sync bidirectionnelle — règles

### 3.1 Source de vérité

| Donnée | Source primaire | Secondaire |
|--------|----------------|------------|
| Agents custom | Dernière modification (updated_at) | Sync des deux côtés |
| Skills L3 | ESP32 FATFS | Sync vers PWA |
| Mémoire L2 (MEMORY.md) | ESP32 FATFS | Copie dans PWA IndexedDB |
| Rappels | ESP32 FATFS | Sync vers PWA IndexedDB |
| Config apps (actives, clés) | PWA localStorage | Push vers ESP32 NVS |
| Clés API | PWA localStorage | Push vers ESP32 NVS |
| Mode silencieux | NVS ESP32 | Sync vers PWA |

### 3.2 Fréquence

- À la connexion BLE : sync complète automatique.
- Durant la session : sync incrémentale sur événement (nouvel agent, nouveau rappel...).
- Jardinier hebdo : passe de consolidation sur les deux côtés.

---

## 4. Décisions ouvertes (TODO)

- [ ] Choisir entre Arduino + ESP-IDF component ou full ESP-IDF pour le firmware (impacte ULP/wake word).
- [ ] Valider le hardware audio : micro I2S (INMP441 ?) + DAC/amp (MAX98357 ?).
- [ ] Définir les UUIDs BLE définitifs pour les 8 caractéristiques.
- [ ] Décider si Musique passe par BLE relay (streaming audio) ou A2DP direct.
- [ ] Définir le protocole de fragmentation BLE pour les payloads > MTU (en cours dans ble_protocol.js).
