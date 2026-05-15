# CompagnonV2 — Spécification fonctionnelle complète

> Version v2.4 — mai 2026  
> Fusion de Compagnon (PWA + firmware) et Compagnon2 (voice OS + agents mimiclaw)

---

## 0. Stack technique de référence

| Composant | Version choisie | Raison |
|-----------|----------------|--------|
| **Arduino ESP32** | **3.3.8** | Dernière stable, basée sur IDF 5.5.4 |
| **LVGL** | **9.x** | Version 9 supportée par Arduino ESP32 3.3.8, API moderne |
| **Build system** | **PlatformIO** (platformio.ini) | Gestion précise des versions de librairies |
| **Arduino_GFX_Library** | dernière stable | Driver CO5300 QSPI (moononournation) |
| **SensorLib** | dernière stable | Touch **CST9220** + RTC PCF85063 + IMU QMI8658 |
| **XPowersLib** | **0.2.x** | Pilote officiel AXP2101 |
| **ArduinoJson** | **7.3.x** | API moderne, zéro copie |

> **Note LVGL 9** : l'API a changé par rapport à LVGL 8 :
> `lv_display_create()`, `lv_indev_set_type()`, `lv_draw_buf_t` (plus de `lv_disp_draw_buf_t`).
> Tout le code UI est à écrire directement en API v9 — ne pas recycler de code v8.

> **Note sur les clés API** : aucune clé n'est stockée dans le firmware.
> Toutes les clés (Groq, OpenWeatherMap, etc.) sont saisies dans la PWA
> et poussées via BLE → stockées en NVS namespace `api_keys`.
> `config/secrets_template.h` ne contient qu'un SSID de dev optionnel
> (vide en production) — uniquement pour faciliter le développement.
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
- `secrets_template.h` = **dev uniquement** (SSID optionnel), jamais de clé API.

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
    ├── platformio.ini     ← Arduino 3.3.8 + LVGL 9.x
    ├── partitions/
    │   └── compagnon_16mb.csv
    └── src/
        ├── main.cpp
        ├── config/
        │   ├── pins.h             ← pins officielles Waveshare (3 boutons inclus)
        │   ├── ui_config.h        ← safe area BORDER_H=20, BORDER_V=10
        │   ├── lv_conf.h          ← LVGL 9.x
        │   └── secrets_template.h ← dev uniquement, NO clés API
        ├── hal/
        │   ├── display.h/.cpp     ← CO5300 QSPI (Arduino_GFX)
        │   ├── touch.h/.cpp       ← CST9220 I2C (SensorLib)
        │   ├── pmu.h/.cpp         ← AXP2101 (XPowersLib) + pmu_event_t
        │   ├── rtc.h/.cpp         ← PCF85063 I2C (SensorLib)
        │   └── audio_io.h/.cpp    ← ES7210 (mic) + ES8311 (DAC) I2S
        ├── net/
        ├── system/
        ├── voice/
        ├── agent_brain/
        ├── storage/
        ├── apps/
        └── ui/
```

---

## 3. Hardware — Pins et boutons

### 3.1 Pins (source officielle Waveshare ESP32-S3-Touch-AMOLED-2.16)

| Périphérique | Pins |
|---|---|
| Display CO5300 QSPI | SDIO0=4, SDIO1=5, SDIO2=6, SDIO3=7, SCLK=38, RST=2, CS=12 |
| I2C bus (partagé) | SDA=15, SCL=14 |
| Touch **CST9220** | INT=11, sur I2C bus (SDA=15, SCL=14) |
| IMU QMI8658 | sur I2C bus |
| RTC PCF85063 | sur I2C bus |
| PMU AXP2101 | sur I2C bus, IRQ = bouton PWR |
| Audio ES7210 (mic) | BCLK=9, LRCK=45, DIN=10, MCLK=16 |
| Audio ES8311 (DAC) | DOUT=8, BCLK/LRCK/MCLK partagés |
| Ampli PA | GPIO46 |
| Bouton + (KEY) | GPIO18 |
| Bouton − (BOOT) | GPIO0 |
| Bouton PWR | AXP2101 I2C IRQ |

> Display physique : 466×466 px (CO5300). Référence LVGL : 480×480 px virtuel.
> Les coordonnées CO5300 doivent être paires — `display_rounder_cb()` s'en charge.

### 3.2 Mapping boutons

| Bouton | Appui court | Appui long |
|--------|-------------|------------|
| **+** GPIO18 | Tile suivante (carousel →) | Lancer l'app sélectionnée |
| **−** GPIO0 | Tile précédente (carousel ←) | Retour / quitter l'app active |
| **PWR** AXP IRQ | Toggle backlight (PKEY_SHORT_IRQ) | Arrêt complet `pmu_poweroff()` (PKEY_LONG_IRQ) |

Aucun conflit : touch CST9220 sur INT GPIO11 (I2C), IMU/RTC sur I2C, boutons sur GPIO indépendants.

### 3.3 pmu_event_t

```cpp
typedef enum {
    PMU_EVT_NONE,
    PMU_EVT_PWR_SHORT,  // → toggle backlight
    PMU_EVT_PWR_LONG,   // → pmu_poweroff()
} pmu_event_t;
```

---

## 4. Compagnon OS — Kernel FreeRTOS dual-core

### 4.1 Mapping des cores

| Core | Tâches |
|------|--------|
| Core 0 | wake word (ESP-SR), audio I2S capture/playback, BLE, WiFi, timers réseau |
| Core 1 | LVGL 9 render, touch CST9220/boutons, OS main tick, agent brain ReAct, orchestrateur apps |

### 4.2 Tâches FreeRTOS

| Tâche | Core | Priorité |
|-------|------|----------|
| task_ui_lvgl | 1 | HAUTE |
| task_os_main | 1 | NORMALE |
| task_voice_io | 0 | HAUTE |
| task_ble | 0 | NORMALE |
| task_network | 0 | BASSE |
| task_agent_brain | 1 | BASSE |

### 4.3 RAM apps

- Interface `AppBase` : `init()` / `start()` / `stop()` / `tick()`.
- `start()` = alloue UI LVGL 9 + buffers.
- `stop()` = libère tout → RAM minimale quand aucune app n'est active.

### 4.4 Light sleep

Réveil : wake word (ESP-SR Core 0), timer RTC PCF85063 (rappels), bouton physique, touch CST9220.

---

## 5. Réseau

### 5.1 WiFi — provisioning via PWA

1. ESP32 scanne → `WIFI_SCAN` BLE → PWA affiche liste.
2. PWA saisit mdp → `WIFI_PROVISION` BLE → ESP32 connecte + NVS.
3. Fallback last resort : AP `Compagnon_Setup` timeout 180s.

### 5.2 BLE — 8 caractéristiques

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

## 6. UI LVGL 9 — Safe area + Status bar + Launcher

### 6.1 Safe area (boîtier arrondi)

```
Ecran physique : 466 × 466 px (CO5300)
Référence LVGL : 480 × 480 px virtuel
BORDER_H = 20 px  (gauche ET droite)
BORDER_V = 10 px  (haut ET bas)
Zone logique : x=20, y=10, w=440, h=460 px
Status bar   : x=20, y=10, w=440, h=36 px
Zone apps    : x=20, y=46, w=440, h=424 px
```

Définis dans `config/ui_config.h`.

### 6.2 Status bar

- Date/heure : `"15 mai 2026 - 16:05"` (FR, NTP/RTC PCF85063, TZ Europe/Paris).
- Icône BLE (si apparié) + icône WiFi (si connecté).
- Jauge batterie : fill coloré (vert > 30%, orange 15–30%, rouge < 15%), % DANS la jauge.
- Données batterie via `pmu_battery_percent()` / `pmu_is_charging()`.
- Implémenté avec `lv_label_t`, `lv_bar_t`, `lv_obj_set_style_*` en **API LVGL 9**.

### 6.3 Launcher carousel

- `lv_tileview` 8 tiles, swipe tactile CST9220 + boutons GPIO18 (+) / GPIO0 (−).
- Appui long GPIO18 → `app_start()` de la tile active.
- Appui long GPIO0 → retour / `app_stop()`.

---

## 7. Voice

- Wake word : **ESP-SR**, Core 0, depuis light sleep.
- STT : **Groq Whisper** (WiFi) ou BLE relay.
- TTS : **Groq PlayAI** → Web Speech via BLE → sons I2S ES8311.
- Bouton micro LVGL 9 dans Nestor + Rappels.
- Mode silencieux : flag NVS (configurable PWA).

---

## 8. App Rappels

- Création : formulaire PWA, texte libre, ou voix (wake word → STT → agent Rappels).
- Modèle : `id`, `title`, `body`, `datetime`, `advance_minutes`, `repeat`, `status`.
- Réveil : timer RTC PCF85063 → light sleep exit → son ES8311 + TTS + UI LVGL 9.
- Sync : `REMINDERS_SYNC` BLE.

---

## 9. Agent Brain — Nestor

- Base **mimiclaw** (Compagnon2).
- Mémoire **L0-L4** : NVS → FATFS → SD.
- **Jardinier** : cron hebdo + commande vocale.
- **Fabrique** : création agent vocal ou PWA.
- **Cristallisation** : skill auto si workflow ≥ N fois.

---

## 10. Stockage multi-niveaux

| Niveau | Firmware | PWA |
|--------|---------|-----|
| Critique | NVS | localStorage |
| Principal | FATFS flash | IndexedDB |
| Optionnel | SD card | Cache API |

---

## 11. PWA

- Responsive : 375 / 768 / 1024px.
- Vue Compagnon : BLE, WiFi, Apps, Clés API, Agents, Rappels, Prefs.

---

## 12. Roadmap

| Phase | Étape | Statut | Notes |
|-------|-------|--------|-------|
| **Phase 1** | Step 1 : Squelette firmware (platformio.ini Arduino 3.3.8 + LVGL **9.x**, partitions, config, FreeRTOS skeleton) | ✅ | |
| | Step 2 : HAL complet (display CO5300 QSPI, touch **CST9220**, PMU AXP2101 + pmu_event_t, RTC PCF85063, audio ES7210+ES8311) | ✅ | 3 boutons mappés (GPIO18/0/PWR IRQ) |
| | Step 3 : Status bar LVGL **9** (date FR, BLE/WiFi icônes, jauge batterie colorée) | 🔜 | |
| | Step 4 : Launcher carousel 8 apps (tileview LVGL 9, swipe CST9220, boutons GPIO) | 🔜 | |
| | Step 5 : NVS + FATFS + SD optionnel | 🔜 | |
| | Step 6 : WiFi manager | 🔜 | |
| **Phase 2** | Step 7 : BLE 8 caractéristiques | 🔜 | |
| | Step 8 : 7 apps existantes sur AppBase | 🔜 | |
| **Phase 3** | Step 9 : App Rappels | 🔜 | |
| | Step 10 : Wake word + STT/TTS | 🔜 | |
| | Step 11 : Agent brain mimiclaw | 🔜 | |
| **Phase 4** | Step 12 : PWA responsive + Rappels | 🔜 | |
| **Phase 5** | Step 13 : Sync + tests intégration | 🔜 | |
