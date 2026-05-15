# CompagnonV2 — Spécification fonctionnelle complète

> Version v2.2 — mai 2026  
> Fusion de Compagnon (PWA + firmware) et Compagnon2 (voice OS + agents mimiclaw)

---

## 0. Stack technique de référence

| Composant | Version choisie | Raison |
|-----------|----------------|--------|
| **Arduino ESP32** | **3.3.8** | Dernière stable, basée sur IDF 5.5.4 |
| **LVGL** | **8.4.x** | Version 8 éprouvée et stable avec Arduino ESP32 (v9 instable avec Arduino, migration risquée) |
| **Build system** | **PlatformIO** (platformio.ini) | Gestion précise des versions de librairies |
| **ArduinoJson** | **7.3.x** | API moderne, zéro copie |
| **XPowersLib** | **0.2.x** | Pilote officiel AXP2101 |
| **ESP32-S3** | Arduino 3.3.8 | via platformio.ini |

> **Note sur les clés API** : aucune clé n'est stockée dans le firmware.
> Toutes les clés (Groq, OpenWeatherMap, etc.) sont saisies dans la PWA
> et poussées via BLE → stockées en NVS namespace `api_keys`.
> `config/secrets_template.h` ne contient que le nom BLE, le wake word
> et un SSID de dev optionnel (vide en production).
> Tous les paramètres (BLE name, wake word, WiFi, clés API, préfs) sont
> configurables **depuis la PWA uniquement** et poussés via BLE.

---

## 1. Nomenclature et rôles

### 1.1 Définitions

- **Compagnon** = nom du projet dans son ensemble.
- **Compagnon OS** = le firmware ESP32-S3.
- **Compagnon PWA** = la Progressive Web App.
- **Nestor** = une APP de Compagnon, l'assistant IA (1 parmi les 8 apps).
- **Compagnon device** = l'ESP32-S3 avec écran Waveshare AMOLED 2.16".

### 1.2 Les 8 apps

1. Bourse
2. Météo
3. Nestor (assistant IA)
4. Musique
5. Radars
6. SmartHome
7. Ecovacs
8. Rappels ← *nouvelle app*

### 1.3 Tout se configure depuis la PWA

- Nom BLE, wake word, WiFi, clés API, préférences, apps actives, agents.
- Le firmware ne contient **aucun secret** ni valeur par défaut métier codée en dur.

### 1.4 Indépendance et sync

- PWA tourne seule (cloud) sans ESP32.
- Firmware tourne seul (WiFi direct) avec fallback BLE relay.
- Sync bidirectionnelle PWA ↔ ESP32 via BLE.

---

## 2. Structure du repo

```text
CompagnonV2/
├── README.md
├── SPEC.md
├── ARCHITECTURE.md
├── pwa/
│   └── src/
│       ├── bt/            (ble.js, ble_protocol.js)
│       ├── core/          (orchestrateur, agent engine, TTS cascade)
│       ├── device/        (provisioning WiFi, device_settings)
│       └── ui/            (app-shell, launcher, 8 app-views)
└── firmware/
    ├── platformio.ini     ← Arduino 3.3.8 + LVGL 8.4.x
    ├── partitions/
    │   └── compagnon_16mb.csv
    └── src/
        ├── main.cpp
        ├── config/
        │   ├── pins.h
        │   ├── ui_config.h        ← safe area BORDER_H=20, BORDER_V=10
        │   ├── lv_conf.h          ← LVGL 8.4.x
        │   └── secrets_template.h ← NO clés API (via PWA BLE)
        ├── hal/
        │   ├── display.h/.cpp     ← rm67162 QSPI
        │   ├── touch.h/.cpp       ← CST816S I2C
        │   ├── pmu.h/.cpp         ← AXP2101 (XPowersLib)
        │   ├── rtc.h/.cpp         ← PCF85063 I2C
        │   └── audio_io.h/.cpp    ← ES8311 I2S
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
| Core 0 | wake word (ESP-SR), audio I2S capture/playback, BLE, WiFi, timers réseau |
| Core 1 | LVGL render, touch/boutons, OS main tick, agent brain ReAct, orchestrateur apps |

### 3.2 Tâches FreeRTOS

| Tâche | Core | Priorité |
|-------|------|----------|
| task_ui_lvgl | 1 | HAUTE |
| task_os_main | 1 | NORMALE |
| task_voice_io | 0 | HAUTE |
| task_ble | 0 | NORMALE |
| task_network | 0 | BASSE |
| task_agent_brain | 1 | BASSE |

### 3.3 RAM apps

- Interface `AppBase` : `init()` / `start()` / `stop()` / `tick()`.
- `start()` = alloue UI LVGL + buffers.
- `stop()` = libère tout.

### 3.4 Light sleep

Réveil : wake word, timer RTC (rappels), bouton physique, touch screen.

---

## 4. Réseau

### 4.1 WiFi — provisioning via PWA

1. ESP32 scanne → `WIFI_SCAN` BLE → PWA affiche liste.
2. PWA saisit mdp → `WIFI_PROVISION` BLE → ESP32 connecte + NVS.
3. Fallback last resort : AP `Compagnon_Setup` timeout 180s.

### 4.2 BLE — 8 caractéristiques

| Caractéristique | Rôle |
|----------------|------|
| GPS | lat,lon depuis téléphone |
| WIFI_SCAN | scan → JSON |
| WIFI_PROVISION | {ssid, pwd} → NVS |
| AGENT_SYNC | agents/skills/mémoire |
| TEXT_INPUT | clavier distant |
| LLM_RELAY | relay LLM via smartphone |
| DEVICE_STATUS | batterie, mode, silencieux |
| REMINDERS_SYNC | CRUD rappels |

---

## 5. UI LVGL 8 — Safe area + Status bar + Launcher

### 5.1 Safe area (boîtier arrondi)

```
Ecran physique : 480 × 480 px
BORDER_H = 20 px  (gauche ET droite)
BORDER_V = 10 px  (haut ET bas)
Zone logique : x=20, y=10, w=440, h=460 px
Status bar   : x=20, y=10, w=440, h=36 px
Zone apps    : x=20, y=46, w=440, h=424 px
```

Définis dans `config/ui_config.h`.

### 5.2 Status bar

- Date/heure : `"15 mai 2026 - 16:05"` (FR, NTP).
- Icône BLE (si apparié) + icône WiFi (si connecté).
- Jauge batterie : fill coloré (vert > 30%, orange 15-30%, rouge < 15%), % DANS la jauge.

### 5.3 Launcher carousel

- `lv_tileview` 8 tiles, swipe + boutons LEFT/RIGHT.
- Appui long RIGHT → `app_start()`.

---

## 6. Voice

- Wake word : **ESP-SR**, Core 0, depuis light sleep.
- STT : **Groq Whisper** (WiFi) ou BLE relay.
- TTS : **Groq PlayAI** → Web Speech via BLE → sons I2S.
- Bouton micro LVGL dans Nestor + Rappels.
- Mode silencieux : flag NVS.

---

## 7. App Rappels

- Création : formulaire PWA, texte libre, ou voix.
- Modèle : `id`, `title`, `body`, `datetime`, `advance_minutes`, `repeat`, `status`.
- Réveil : timer RTC → light sleep exit → son + TTS + UI.
- Sync : `REMINDERS_SYNC` BLE.

---

## 8. Agent Brain — Nestor

- Base **mimiclaw** (Compagnon2).
- Mémoire **L0-L4** : NVS → FATFS → SD.
- **Jardinier** : cron hebdo + commande vocale.
- **Fabrique** : création agent vocal ou PWA.
- **Cristallisation** : skill auto si workflow ≥ N fois.

---

## 9. Stockage multi-niveaux

| Niveau | Firmware | PWA |
|--------|---------|-----|
| Critique | NVS | localStorage |
| Principal | FATFS flash | IndexedDB |
| Optionnel | SD card | Cache API |

---

## 10. PWA

- Responsive : 375 / 768 / 1024px.
- Vue Compagnon : BLE, WiFi, Apps, Clés API, Agents, Rappels, Prefs.

---

## 11. Roadmap

| Phase | Étape | Statut |
|-------|-------|--------|
| **Phase 1** | Step 1 : Squelette firmware (Arduino **3.3.8**, LVGL **8.4**, partitions, config, FreeRTOS) | ✅ |
| | Step 2 : HAL complet (display, touch, PMU, RTC, audio) | ✅ |
| | Step 3 : Status bar LVGL 8 | 🔜 |
| | Step 4 : Launcher carousel 8 apps | 🔜 |
| | Step 5 : NVS + FATFS + SD optionnel | 🔜 |
| | Step 6 : WiFi manager | 🔜 |
| **Phase 2** | Step 7 : BLE 8 caractéristiques | 🔜 |
| | Step 8 : 7 apps existantes sur AppBase | 🔜 |
| **Phase 3** | Step 9 : App Rappels | 🔜 |
| | Step 10 : Wake word + STT/TTS | 🔜 |
| | Step 11 : Agent brain mimiclaw | 🔜 |
| **Phase 4** | Step 12 : PWA responsive + Rappels | 🔜 |
| **Phase 5** | Step 13 : Sync + tests intégration | 🔜 |
