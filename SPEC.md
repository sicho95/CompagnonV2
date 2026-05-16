# CompagnonV2 — Spécification fonctionnelle complète

> Version v2.6 — mai 2026  
> Fusion de Compagnon (PWA + firmware) et Compagnon2 (voice OS + agents mimiclaw)

---

## 0. Stack technique de référence

| Composant | Version choisie | Raison |
|-----------|----------------|--------|
| **Arduino ESP32** | **3.3.8** | Dernière stable, basée sur IDF 5.5.4 |
| **LVGL** | **9.x** | Version 9 supportée par Arduino ESP32 3.3.8, API moderne |
| **Build system** | **Arduino IDE / PlatformIO** | platformio.ini ou sketch selon workflow |
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
        │   ├── pins.h             ← TOUTES les pins vérifiées schéma rev2
        │   ├── ui_config.h        ← safe area BORDER_H=20, BORDER_V=10
        │   ├── lv_conf.h          ← LVGL 9.x
        │   └── secrets_template.h ← dev uniquement, NO clés API
        ├── hal/
        │   ├── display.h/.cpp     ← CO5300 QSPI (Arduino_GFX)
        │   ├── touch.h/.cpp       ← CST9220 I2C (SensorLib)
        │   ├── pmu.h/.cpp         ← AXP2101 (XPowersLib) + pmu_event_t
        │   ├── rtc.h/.cpp         ← PCF85063 I2C (SensorLib)
        │   └── hal_audio.h/.cpp   ← ES7210 4-mic I2S RX + NS4150B PA_EN
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

### 3.1 Pins (vérifiées sur schéma officiel Waveshare ESP32-S3-Touch-AMOLED-2.16 rev2)

| Périphérique | Signal | GPIO | Notes |
|---|---|---|---|
| **Display CO5300** | LCD_CS | 12 | QSPI chip select |
| | QSPI_SIO0 | 4 | Data 0 |
| | QSPI_SI1 | 5 | Data 1 |
| | QSPI_SI2 | 6 | Data 2 |
| | QSPI_SI3 | 7 | Data 3 |
| | QSPI_SCL | **38** | ⚠️ partagé ES7210 MCLK — init display AVANT audio |
| | LCD_RESET | 39 | |
| | LCD_TE | — | non connecté à un GPIO numéroté |
| **I2C bus partagé** | SDA | 15 | Touch + IMU + RTC + PMU |
| | SCL | 14 | Touch + IMU + RTC + PMU |
| **Touch CST9220** | TP_INT | **11** | IRQ touch |
| | TP_RESET | **40** | Reset actif bas |
| | I2C addr | 0x1A | sur bus partagé |
| **IMU QMI8658** | INT1 | **17** | wake/geste |
| | INT2 | **21** | (optionnel) |
| | I2C addr | 0x6B | sur bus partagé |
| **RTC PCF85063** | — | — | sur bus partagé, addr 0x51 |
| **PMU AXP2101** | IRQ | **9** | sur bus partagé, addr 0x34 |
| **Audio ADC ES7210** | MCLK | **38** | ⚠️ partagé LCD_QSPI_SCL |
| | BCLK | 36 | I2S RX |
| | LRCK | 35 | I2S RX |
| | DIN (ASDOUT) | **10** | SDOUT1/TDMOUT → GPIO10 via 51Ω |
| | I2C addr | 0x40 | sur bus partagé |
| **Speaker NS4150B** | PA_EN (CTRL) | **46** | HIGH=actif, LOW=shutdown |
| | Entrée audio | analogique | DAC ES7210 → NS4150B PA_INL+/- directement |
| | **Pas de I2S TX** | — | Ampli analogique, aucun I2S TX côté ESP32 |
| **Bouton BOOT** | Key2 | **0** | pull-up 10K, actif bas, wake EXT1 |
| **Bouton USER** | Key3 | **18** | pull-up 10K, actif bas |
| **Power PWRON** | Key1 | AXP IRQ | géré par PMU AXP2101 |
| **Power latch** | SYS_OUT | **16** | BSS138 transistor — HIGH pour maintenir alim |
| **SD Card** | MOSI | 1 | SPI |
| | SCK | 2 | SPI |
| | MISO | 3 | SPI |
| | CS (SDCS) | **41** | |

> **Note GPIO38** : partagé entre `QSPI_SCL` (LCD) et `ES7210_MCLK` (audio).
> Probablement intentionnel côté Waveshare (MCLK dérivé du clock QSPI).
> Règle d'init : **initialiser le display AVANT l'audio**.

> **Note NS4150B** : l'ampli speaker est **100% analogique**.
> Le signal audio sort du DAC interne de l'ES7210 vers les broches PA_INL+/PA_INL-
> puis vers l'entrée IN+/IN- du NS4150B. Il n'y a **aucun I2S TX** à configurer
> côté ESP32. La seule action firmware = `GPIO46 HIGH` pour activer l'ampli.

### 3.2 Mapping boutons

| Bouton | GPIO | Appui court | Appui long |
|--------|------|-------------|------------|
| **USER** | 18 | Tile suivante (carousel →) | Lancer l'app sélectionnée |
| **BOOT** | 0 | Tile précédente (carousel ←) | Retour / quitter l'app active |
| **PWR** | AXP IRQ | Toggle screen (PKEY_SHORT_IRQ) | Arrêt `pmu_poweroff()` (PKEY_LONG_IRQ) |

### 3.3 pmu_event_t

```cpp
typedef enum {
    PMU_EVT_NONE,
    PMU_EVT_PWR_SHORT,  // → toggle screen
    PMU_EVT_PWR_LONG,   // → pmu_poweroff()
} pmu_event_t;
```

---

## 4. Compagnon OS — Kernel FreeRTOS dual-core

### 4.1 Mapping des cores

| Core | Tâches |
|------|--------|
| Core 0 | wake word (ESP-SR), audio I2S capture (ES7210), BLE, WiFi, timers réseau |
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

Sources de réveil :
- Wake word (ESP-SR, Core 0)
- Timer RTC PCF85063 (rappels)
- Bouton BOOT GPIO0 (EXT1)
- AXP2101 IRQ GPIO9 (bouton power)
- Touch CST9220 INT GPIO11 (option)

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
Ecran physique CO5300 : 480 × 480 px
                        ↑ résolution réelle — le boîtier masque les bords

Marges boîtier :
  BORDER_H = 20 px  (marge gauche ET droite — chacune)
  BORDER_V = 10 px  (marge haut ET bas — chacune)

Zone affichable (safe area) :
  Origine : x = BORDER_H = 20,  y = BORDER_V = 10
  Taille  : w = 480 - 2×20 = 440 px,  h = 480 - 2×10 = 460 px

Status bar (en haut de la safe area) :
  x=20, y=10, w=440, h=36 px

Zone apps (sous la status bar, dans la safe area) :
  x=20, y=46, w=440, h=424 px  (460 - 36)
```

Toutes ces valeurs sont définies dans `config/ui_config.h` via les macros :
`SCREEN_W`, `SCREEN_H`, `BORDER_H`, `BORDER_V`, `UI_X`, `UI_Y`, `UI_W`, `UI_H`,
`STATUS_BAR_H`, `APP_X`, `APP_Y`, `APP_W`, `APP_H`.

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

- Wake word : **ESP-SR**, Core 0, depuis light sleep, détection sur GPIO10 (ES7210 ASDOUT).
- STT : **Groq Whisper** (WiFi) ou BLE relay si pas de WiFi.
- TTS : **Groq PlayAI** → Web Speech via BLE relay → sons basiques si hors ligne.
  - TTS audio joué via NS4150B (PA_EN=GPIO46 HIGH avant lecture).
- Bouton micro LVGL 9 dans les apps Nestor + Rappels.
- Mode silencieux : flag NVS `silent_mode` (configurable PWA), bypass TTS sauf alertes critiques.

---

## 8. App Rappels

- Création : formulaire PWA, texte libre, ou voix (wake word → STT → agent Rappels).
- Modèle : `id`, `title`, `body`, `datetime`, `advance_minutes`, `repeat`, `status`.
- Réveil : timer RTC PCF85063 → light sleep exit → PA_EN HIGH + son + TTS + UI LVGL 9.
- Sync : `REMINDERS_SYNC` BLE.
- Acknowledge : UI affiche "Fait" / "Plus tard" (snooze).

---

## 9. Agent Brain — Nestor

- Base **mimiclaw** (Compagnon2).
- Mémoire **L0-L4** : NVS → FATFS → SD.
- **Jardinier** : cron hebdo + commande vocale — nettoyage skills, compactage mémoire.
- **Fabrique** : création agent vocal ou PWA à la volée.
- **Cristallisation** : skill auto si workflow répété ≥ N fois.
- **Tâches planifiées** : scheduler interne (cron simple NVS-backed).

---

## 10. Stockage multi-niveaux

| Niveau | Firmware | PWA |
|--------|---------|-----|
| Critique (params, clés, flags) | NVS | localStorage |
| Principal (agents, mémoire, rappels, sessions) | FATFS flash | IndexedDB |
| Optionnel (archives, logs) | SD card (GPIO1/2/3/41) | Cache API |

---

## 11. PWA

- Responsive : 375 / 768 / 1024px.
- Vue Compagnon : BLE, WiFi, Apps, Clés API, Agents, Rappels, Prefs.
- Provisioning WiFi : WIFI_SCAN → WIFI_PROVISION.
- Toutes les configs poussées via BLE (jamais hardcodées firmware).

---

## 12. Roadmap

| Phase | Étape | Statut | Notes |
|-------|-------|--------|-------|
| **Phase 1** | Step 1 : Squelette firmware (Arduino 3.3.8 + LVGL 9.x, partitions, config, FreeRTOS skeleton) | ✅ | |
| | Step 2 : HAL complet (display CO5300 QSPI, touch CST9220, PMU AXP2101, RTC PCF85063, audio ES7210+NS4150B) | ✅ | Pins toutes vérifiées schéma rev2 — pas d'I2S TX (ampli analogique) |
| | Step 3 : Status bar LVGL 9 (date FR, BLE/WiFi icônes, jauge batterie colorée) | ✅ | Présent dans ui/status_bar.h/.cpp |
| | Step 4 : Launcher carousel 8 apps (tileview LVGL 9, swipe CST9220, boutons GPIO18/0) | ✅ | Présent dans ui/launcher.h/.cpp |
| | Step 5 : NVS + FATFS + SD optionnel | 🔜 | nvs_store.h/.cpp + reminder_store.h/.cpp à créer |
| | Step 6 : WiFi manager + provisioning BLE | 🔜 | wifi_mgr.cpp manquant |
| **Phase 2** | Step 7 : BLE 8 caractéristiques | ✅ | net/ble_manager.h/.cpp présent |
| | Step 7b : http_client HTTPS Groq (TLS cert bundle) | 🔜 | net/http_client.h/.cpp à créer |
| | Step 8 : 7 apps existantes portées sur AppBase | 🔜 | app_base.h à créer, squelettes présents |
| **Phase 3** | Step 9 : App Rappels complète | 🔜 | ui_reminders présent, reminder_store manquant |
| | Step 10 : Wake word + STT/TTS (ESP-SR + Groq) | 🔜 | voice_engine.h/.cpp présent, http_client requis |
| | Step 11 : Agent brain mimiclaw + mémoire L0-L4 | 🔜 | |
| **Phase 4** | Step 12 : PWA responsive + Rappels + sync | 🔜 | |
| **Phase 5** | Step 13 : Tests intégration + OTA | 🔜 | |
