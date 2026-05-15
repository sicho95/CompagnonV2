# CompagnonV2 — Spécification fonctionnelle complète

> Version v2.1 — mai 2026  
> Fusion de Compagnon (PWA + firmware) et Compagnon2 (voice OS + agents mimiclaw)

---

## 0. Stack technique de référence

| Composant | Version choisie | Raison |
|-----------|----------------|--------|
| **Arduino ESP32** | **3.3.8** | Dernière stable, basée sur IDF 5.5 |
| **LVGL** | **9.3.0** | Dernière stable LV9, compatible Arduino |
| **Build system** | **PlatformIO** (platformio.ini) | Gestion des dépendances + versions précises |
| **ArduinoJson** | **7.3.x** | API moderne, zéro copie |
| **XPowersLib** | **0.2.x** | Pilote officiel AXP2101 |
| **ESP32-S3** | Arduino framework | via platformio.ini |

> **Note sur les clés API** : aucune clé n'est stockée dans le firmware.
> Toutes les clés (Groq, OpenWeatherMap, etc.) sont saisies dans la PWA
> et poussées via BLE → stockées en NVS namespace `api_keys`.
> `config/secrets_template.h` ne contient que le nom BLE, le wake word
> et un SSID de dev optionnel (vide en production).

---

## 1. Nomenclature et rôles

### 1.1 Définitions

- **Compagnon** = nom du projet dans son ensemble.
- **Compagnon OS** = le firmware ESP32-S3 (cet OS s'appelle Compagnon OS).
- **Compagnon PWA** = la Progressive Web App (UI, config, hub cloud).
- **Nestor** = une APP de Compagnon, l'assistant IA (1 parmi les 8 apps).
- **Compagnon device** = l'ESP32-S3 avec écran Waveshare AMOLED 2.16".

### 1.2 Les 8 apps (identiques PWA + firmware)

1. Bourse
2. Météo
3. Nestor (assistant IA embarqué)
4. Musique
5. Radars
6. SmartHome (Maison)
7. Ecovacs
8. Rappels ← *nouvelle app à créer*

### 1.3 Configuration depuis la PWA

Tout ce qui est configurable l'est **depuis la PWA uniquement** :
- Nom BLE du device
- Wake word
- WiFi (scan + provisioning via BLE)
- Clés API
- Préférences (mode silencieux, luminosité, langue, timezone)
- Apps actives
- Agents

Le firmware ne contient **aucun secret** ni valeur par défaut métier codée en dur.
Les seuls defines de `secrets_template.h` sont des valeurs de dev optionnelles.

### 1.4 Indépendance et sync

- La **PWA tourne seule** sans ESP32 : toutes les apps y fonctionnent en mode cloud.
- Le **firmware tourne seul** sans téléphone : apps fonctionnent via WiFi direct, avec fallback BLE relay si pas de WiFi.
- La **sync bidirectionnelle** PWA ↔ ESP32 via BLE.

---

## 2. Structure du repo

```text
CompagnonV2/
├── README.md
├── SPEC.md
├── ARCHITECTURE.md
├── pwa/
│   ├── index.html
│   ├── manifest.json
│   ├── service-worker.js
│   ├── css/
│   └── src/
│       ├── app.js
│       ├── api/
│       ├── bt/            (ble.js, ble_protocol.js, ble_status.js)
│       ├── core/          (orchestrateur, agent engine, TTS cascade)
│       ├── device/        (provisioning WiFi, device_settings, device_config)
│       ├── input/
│       ├── storage/
│       ├── sync/
│       └── ui/
│           ├── app-shell.js
│           ├── launcher.js
│           ├── compagnon.js  (config ESP32 : BLE, WiFi, agents, batterie)
│           └── [app]-view.js × 8
└── firmware/
    ├── platformio.ini        ← Arduino 3.3.8 + LVGL 9.3.0
    ├── partitions/
    │   └── compagnon_16mb.csv
    └── src/
        ├── main.cpp
        ├── config/
        │   ├── pins.h
        │   ├── ui_config.h   ← safe area BORDER_H=20, BORDER_V=10
        │   ├── lv_conf.h     ← LVGL 9.3.0
        │   └── secrets_template.h  ← NO clés API (via PWA BLE)
        ├── hal/
        │   ├── display.h/.cpp   ← rm67162 QSPI
        │   ├── touch.h/.cpp     ← CST816S I2C
        │   ├── pmu.h/.cpp       ← AXP2101 (XPowersLib)
        │   ├── rtc.h/.cpp       ← PCF85063 I2C
        │   └── audio_io.h/.cpp  ← ES8311 I2S
        ├── net/
        ├── system/
        ├── voice/
        ├── agent_brain/
        ├── storage/
        ├── apps/
        └── ui/
```

---

## 3. Compagnon OS — Kernel FreeRTOS dual-core

### 3.1 Mapping des cores

| Core | Tâches |
|------|--------|
| Core 0 | wake word (ESP-SR), capture audio I2S, TTS playback, BLE stack, WiFi stack, timers réseau |
| Core 1 | LVGL render, input touch/boutons, OS main tick, agent brain ReAct, orchestrateur apps, schedulers |

### 3.2 Tâches FreeRTOS

```
task_ui_lvgl        (Core 1, priorité HAUTE)
task_os_main        (Core 1, priorité NORMALE)
task_voice_io       (Core 0, priorité HAUTE)
task_ble            (Core 0, priorité NORMALE)
task_network        (Core 0, priorité BASSE)
task_agent_brain    (Core 1, priorité BASSE)
```

### 3.3 Gestion RAM des apps

- Interface `AppBase` : `init()` / `start()` / `stop()` / `tick()`.
- `start()` = alloue LVGL UI + buffers.
- `stop()` = libère tout (aucune RAM résiduelle hors launcher + status bar).

### 3.4 Light sleep et réveil

Sources de réveil : wake word, timer RTC (rappels), bouton physique, touch screen.

---

## 4. Réseau : WiFi, BLE et fallback

### 4.1 WiFi — provisioning via PWA

1. ESP32 scanne → `WIFI_SCAN` BLE → PWA affiche la liste.
2. PWA saisit mdp → `WIFI_PROVISION` BLE → ESP32 connecte + NVS.
3. Fallback last resort : portail AP `Compagnon_Setup` timeout 180s.

### 4.2 BLE — service Compagnon

| Caractéristique | Rôle |
|----------------|------|
| GPS | lat,lon depuis téléphone |
| WIFI_SCAN | scan réseaux → JSON |
| WIFI_PROVISION | {ssid, pwd} → connexion NVS |
| AGENT_SYNC | agents/skills/mémoire bidirectionnel |
| TEXT_INPUT | texte clavier distant |
| LLM_RELAY | relay LLM smartphone (fallback WiFi) |
| DEVICE_STATUS | batterie, mode, silencieux |
| REMINDERS_SYNC | CRUD rappels bidirectionnel |

---

## 5. UI LVGL — Safe area + Status bar + Launcher

### 5.1 Safe area (boîtier arrondi)

```
Ecran physique : 480 × 480 px
BORDER_H = 20 px  (marge gauche ET droite)
BORDER_V = 10 px  (marge haut ET bas)
Zone logique : x=20, y=10, w=440, h=460 px
Status bar   : x=20, y=10, w=440, h=36 px
Zone apps    : x=20, y=46, w=440, h=424 px
```

Tous définis dans `config/ui_config.h` via `BORDER_H`, `BORDER_V`, `UI_X/Y/W/H`, `APP_X/Y/W/H`, `STATUS_BAR_H`.

### 5.2 Status bar

- Date/heure : `"15 mai 2026 - 16:05"` (locale FR, NTP).
- Icône BLE (si apparié) + icône WiFi (si connecté).
- Jauge batterie : contour pile, fill coloré (vert > 30%, orange 15-30%, rouge < 15%), % affiché DANS la jauge.

### 5.3 Launcher carousel

- Tileview LVGL 9 × 8 tiles.
- Navigation swipe + boutons LEFT/RIGHT.
- Appui long RIGHT → `app_start()`.

---

## 6. Voice : wake word, STT, TTS

- Wake word : **ESP-SR**, continu Core 0, y compris depuis light sleep.
- STT : **Groq Whisper** (WiFi) ou BLE relay (fallback).
- TTS : **Groq PlayAI** → fallback Web Speech via BLE → fallback sons I2S.
- Bouton micro LVGL dans Nestor + Rappels.
- Mode silencieux : flag NVS, configurable PWA.

---

## 7. App Rappels

- Création : formulaire PWA, input texte libre NLP, ou voix (wake word / bouton micro).
- Modèle : `id`, `title`, `body`, `datetime`, `advance_minutes`, `repeat`, `status`.
- Réveil : timer RTC → sortie light sleep → son + TTS + UI (Fait / Snooze).
- Sync bidirectionnelle via `REMINDERS_SYNC` BLE.

---

## 8. Agent Brain — Nestor

- Base **mimiclaw** (Compagnon2) : ReAct loop, tools, agents JSON FATFS.
- Mémoire **L0-L4** : NVS → FATFS → SD optionnelle.
- **Jardinier** : cron hebdo + commande vocale (nettoyage skills/agents).
- **Fabrique** : création agent à la volée par voix ou PWA.
- **Cristallisation** : skill auto si workflow répété ≥ N fois.
- **Crons** : météo 3h, bourse H, radars config, rappels 1min, jardinier hebdo.

---

## 9. Stockage multi-niveaux

| Niveau | Firmware | PWA |
|--------|---------|-----|
| Critique | NVS (WiFi, flags, clés API) | localStorage |
| Principal | FATFS flash (agents, mémoire L0-L4, rappels) | IndexedDB |
| Optionnel | SD card (archives, logs) | Cache API |

SD card = non bloquant si absente.

---

## 10. PWA — Responsive + Config ESP32

- Breakpoints : 375 / 390 / 768 / 1024px.
- Bottom nav (mobile) / Sidebar (desktop) : 8 apps + Compagnon config.
- Vue Compagnon : BLE, WiFi, Apps actives, **Clés API**, Agents, Rappels, Prefs.

---

## 11. Roadmap

| Phase | Étapes |
|-------|--------|
| **Phase 1** | ✅ Step 1 : Squelette firmware (Arduino 3.3.8, LVGL 9.3, partitions, config, FreeRTOS tasks) |
| | ✅ Step 2 : HAL complet (display rm67162, touch CST816S, PMU AXP2101, RTC PCF85063, audio ES8311) |
| | Step 3 : Status bar LVGL (date FR, icônes BLE/WiFi, jauge batterie % dedans) |
| | Step 4 : Launcher carousel 8 apps (tileview LVGL 9) |
| | Step 5 : NVS + FATFS + SD optionnel |
| | Step 6 : WiFi manager (creds NVS, reconnect, scan BLE) |
| **Phase 2** | Step 7 : BLE multi-caractéristiques complet (8 caractéristiques) |
| | Step 8 : Porter 7 apps existantes sur AppBase |
| **Phase 3** | Step 9 : App Rappels firmware + scheduler |
| | Step 10 : Wake word ESP-SR + STT/TTS Groq |
| | Step 11 : Agent brain mimiclaw + Jardinier + Fabrique |
| **Phase 4** | Step 12 : PWA redesign responsive + vue Rappels |
| **Phase 5** | Step 13 : Sync bidirectionnelle complète + tests intégration |
