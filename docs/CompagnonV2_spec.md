# CompagnonV2 — Spécification Firmware

> **Board** : Waveshare ESP32-S3-Touch-AMOLED-2.16"  
> **MCU** : ESP32-S3, 240 MHz, 16 MB Flash, 8 MB PSRAM OPI  
> **Framework** : Arduino 3.3.8 / arduino-esp32  
> **LVGL** : 9.x  
> **Dernière mise à jour** : 2026-05-16

---

## 1. Vue d'ensemble

CompagnonV2 est un firmware pour assistant vocal embarqué sur écran AMOLED tactile. Il repose sur :

- Une interface graphique LVGL (écran CO5300 QSPI, touch CST9220)
- Un pipeline audio complet : capture 4-mic ES7210 (AEC) → STT Groq → LLM Groq → TTS Groq → codec ES8311 → speaker NS4150B
- Un gestionnaire de rappels vocaux planifiés (RTC + FATFS)
- Connectivité WiFi/BLE + OTA

---

## 2. Tableau récapitulatif des GPIO

### 2.1 I2C bus partagé

Tous les périphériques I2C partagent le même bus Wire.

| Signal | GPIO | Périphérique(s) |
|--------|------|-----------------|
| SDA | **GPIO 15** | ES8311 · ES7210 · AXP2101 · QMI8658 · CST9220 |
| SCL | **GPIO 14** | ES8311 · ES7210 · AXP2101 · QMI8658 · CST9220 |

### 2.2 I2S bus partagé (MCLK/SCLK/LRCK)

| Signal | GPIO | Direction | Périphérique |
|--------|------|-----------|--------------|
| MCLK | **GPIO 42** | OUT | ES8311 (MCLK pin 3) · ES7210 (I2S_MCLK) |
| SCLK (BCLK) | **GPIO 9** | OUT | ES8311 (SCLK pin 6) · ES7210 (SCLK pin 9) |
| LRCK (WS) | **GPIO 45** | OUT | ES8311 (LRCK pin 8) · ES7210 (LRCK pin 10) |
| DSDIN TX | **GPIO 8** | OUT → ES8311 | DAC playback TTS (ESP32 → ES8311 DSDIN pin 9) |
| SDOUT1 RX | **GPIO 38** ⚠️ | IN ← ES7210 | Mic capture (ES7210 SDOUT1 → ESP32) — à confirmer |

> ⚠️ GPIO 38 pour ES7210 SDOUT1 est à confirmer sur le schéma complet de la carte.  
> Modifier `PIN_ES7210_DIN` dans `firmware/include/pins.h` si besoin.

### 2.3 Display CO5300 (QSPI)

| Signal | GPIO |
|--------|------|
| CS | GPIO 10 |
| SCK | GPIO 12 |
| D0 | GPIO 11 |
| D1 | GPIO 13 |
| D2 | GPIO 14 |
| D3 | GPIO 9 |
| RST (partagé Touch) | GPIO 2 |

### 2.4 Touch CST9220

| Signal | GPIO |
|--------|------|
| INT | GPIO 4 |
| RST | GPIO 2 (partagé LCD) |

### 2.5 Autres périphériques

| Signal | GPIO | Périphérique |
|--------|------|--------------|
| PA_EN | **GPIO 46** | NS4150B ampli speaker (CTRL) |
| PMU_INT | GPIO 3 | AXP2101 |
| RTC_INT | GPIO 1 | PCF85063 |
| IMU_INT1 | GPIO 44 | QMI8658 |
| IMU_INT2 | GPIO 43 | QMI8658 |
| BOOT_BTN | GPIO 0 | Bouton boot |
| SD_CS | GPIO 5 | SD Card SPI |
| SD_MOSI | GPIO 35 | SD Card SPI |
| SD_CLK | GPIO 36 | SD Card SPI |
| SD_MISO | GPIO 37 | SD Card SPI |

---

## 3. Architecture audio

### 3.1 Schéma de principe

```
MIC1–4 (électret) ──► ES7210 ADC (0x40) ──► I2S RX (I2S_NUM_1, GPIO38)
                           AEC ◄──────────────────────────────────────────┐
                                                                           │
Groq STT ◄── WiFi ◄── ESP32-S3 ◄── mic buffer                            │
Groq LLM → réponse texte                                                  │
Groq TTS → PCM 24kHz 16-bit                                               │
                                                                           │
ESP32-S3 ──► I2S TX (I2S_NUM_0, GPIO8) ──► ES8311 DAC (0x18) ────────────┘
                                                  │
                                            OUTP/OUTN diff
                                                  │
                                           NS4150B PA (GPIO46)
                                                  │
                                              SPEAKER
```

### 3.2 ES7210 — ADC 4-mic avec AEC

| Paramètre | Valeur |
|-----------|--------|
| Adresse I2C | `0x40` (A0=A1=GND) |
| Sample rate | 16 000 Hz |
| Format I2S | Standard 16-bit, TDM 4 slots |
| Gain initial | 18 dB sur MIC1–4 |
| MCLK attendu | 12.288 MHz (GPIO42) |
| Driver | `firmware/lib/es7210/es7210.h/.cpp` |

Registres clés : `0x43` gain MIC1/2, `0x44` gain MIC3/4, `0x45/0x46` bias, `0x4A` power.

### 3.3 ES8311 — Codec DAC/ADC

| Paramètre | Valeur |
|-----------|--------|
| Adresse I2C | `0x18` (ADDR=GND via R28 10K) |
| Sample rate TTS | 24 000 Hz |
| Format I2S | Standard 16-bit, slave |
| Volume par défaut | `0xBF` (0 dB), registre `0x32` |
| MCLK attendu | 12.288 MHz = 24000 × 512 |
| Sortie | Différentielle OUTP/OUTN → NS4150B |
| Driver | `firmware/lib/es8311/es8311.h/.cpp` |

Séquence d'init : reset `0x00` → clock manager (0x01–0x08) → format SDP (0x09/0x0A) → system (0x0D–0x14) → ADC enable (0x1C) → DAC enable (0x31/0x32).

### 3.4 NS4150B — Amplificateur PA

| Paramètre | Valeur |
|-----------|--------|
| Contrôle | GPIO 46 (CTRL, actif HIGH) |
| Entrée | PA_INL+/PA_INL− ← ES8311 OUTP/OUTN |
| Alimentation | VCC3V3 |
| Bypass | Possible via résistance 150K |

### 3.5 API HAL audio (`hal::`)

```cpp
bool   audio_init();                                    // ES7210 + ES8311 + I2S
void   audio_suspend() / audio_resume();                // deep sleep
void   audio_pa_enable(bool en);                        // NS4150B CTRL
size_t audio_mic_read(int16_t* buf, size_t samples);    // capture ES7210
void   audio_set_mic_gain(es7210_input_mic_t, es7210_gain_value_t);
void   audio_set_volume(uint8_t vol);                   // ES8311 DAC vol
void   audio_play_pcm(const uint8_t* buf, size_t len); // TTS → ES8311 → PA
```

`audio_init()` est non-bloquant : si ES7210 ou ES8311 absent sur I2C, log série + skip, le firmware continue.

---

## 4. Structure du projet

```
CompagnonV2/
├── CompagnonV2.ino              — point d'entrée Arduino
├── partitions.csv               — table de partitions custom (16 MB)
└── src/
    ├── hal/
    │   ├── display.h/.cpp       — CO5300 QSPI (LVGL driver)
    │   ├── touch.h/.cpp         — CST9220 I2C
    │   ├── imu.h/.cpp           — QMI8658 I2C (orientation auto)
    │   ├── pmu.h/.cpp           — AXP2101 (rails, bouton power)
    │   ├── rtc.h/.cpp           — PCF85063 (alarmes)
    │   ├── hal_audio.h/.cpp     — pipeline audio complet
    │   └── audio.h              — shim → hal_audio.h
    ├── net/
    │   ├── wifi_mgr.h/.cpp      — connect/retry/AP fallback/NTP
    │   ├── http_client.h/.cpp   — Groq STT/Chat/TTS (HTTPS)
    │   ├── ble_mgr.h/.cpp       — provisioning WiFi + sync clés API
    │   └── ota.h/.cpp           — OTA Arduino
    ├── storage/
    │   ├── reminder_store.h/.cpp — FATFS /reminders.json (ArduinoJson)
    │   └── nvs_store.h/.cpp      — wrappers Preferences NVS
    ├── system/
    │   ├── scheduler.h/.cpp     — planification alarmes RTC + deep sleep
    │   ├── orchestrator.h/.cpp  — boucle principale agent
    │   └── power_mgr.h/.cpp     — light/deep sleep policy
    ├── config/
    │   └── nvs_config.h/.cpp    — clés NVS centralisées
    ├── ui/
    │   ├── launcher.h/.cpp      — écran principal LVGL
    │   ├── status_bar.h/.cpp    — barre état (lv_layer_top)
    │   └── notification_mgr.h/.cpp — toasts LVGL 9
    ├── voice/
    │   └── voice_engine.h/.cpp  — wake word + pipeline STT (Core 0)
    └── apps/
        ├── reminders/           — app rappels vocaux
        ├── smarthome/           — domotique Tuya
        └── ecovacs/             — aspirateur Ecovacs
```

```
firmware/
├── include/
│   └── pins.h                   — mapping GPIO complet (source vérité)
└── lib/
    ├── es7210/
    │   ├── es7210.h             — types + API (registres réels)
    │   └── es7210.cpp           — driver I2C ES7210
    └── es8311/
        ├── es8311.h             — API codec DAC
        └── es8311.cpp           — driver I2C ES8311
```

---

## 5. Pipeline Groq (net/http_client)

| Fonction | Endpoint Groq | Modèle |
|----------|--------------|--------|
| `transcribeAudio()` | `/audio/transcriptions` | `whisper-large-v3-turbo` |
| `chatCompletion()` | `/chat/completions` | `llama-3.3-70b-versatile` |
| `textToSpeech()` | `/audio/speech` | `playai-tts` / voix `Fritz-PlayAI` |

- Transport : `WiFiClientSecure` + bundle CA Mozilla (`x509_crt_bundle_start`)
- Clé API : NVS namespace `"app"`, clé `"groq_api_key"`
- Erreurs : timeout/4xx/5xx → log série + retour vide, pas de crash

---

## 6. Stockage

### 6.1 FATFS — reminder_store

Fichier `/reminders.json` monté sur FFat.

```cpp
struct Reminder {
    int    id;
    String label;
    time_t datetime;        // epoch UTC
    int    advance_minutes;
    bool   enabled;
};
```

CRUD : `load()`, `save()`, `add()`, `remove()`, `update()`, `getAll()`, `getById()`.

### 6.2 NVS — nvs_store

Wrappers centralisés autour de `Preferences`. Aucun accès NVS direct ailleurs.

| Namespace | Contenu |
|-----------|---------|
| `"wifi"` | SSID, password, AP SSID/pass |
| `"app"` | clés API (Groq, Gemini, Serper…) |
| `"system"` | silent_mode, volume… |

---

## 7. Scheduler / RTC

```cpp
struct ScheduledAlarm { int reminder_id; time_t trigger_epoch; };

void scheduleReminder(Reminder& r);   // datetime - advance_minutes*60
void rescheduleAll();                  // recharge tous les rappels actifs
void onWakeup();                       // identifie rappel, lance TTS
void cancelAlarm(int reminder_id);
```

Réveil ESP32 : `esp_sleep_enable_timer_wakeup()` calé sur le prochain `trigger_epoch`.  
`esp_deep_sleep_start()` est commenté dans le code — à activer quand LVGL ne doit pas tourner.

---

## 8. Interface commune AppBase

```cpp
class AppBase {
public:
    virtual void        init()    = 0;
    virtual void        update()  = 0;   // appelé chaque tick LVGL
    virtual void        onResume()= 0;
    virtual void        onPause() = 0;
    virtual const char* getName() = 0;
    virtual ~AppBase()  = default;
};
```

---

## 9. Ordre d'initialisation (setup)

```
1. hal_pmu_init()       — rails AXP2101, bouton power
2. nvs_config_init()    — namespace "compagnon"
3. hal_display_init()   — reset GPIO2, CO5300 QSPI
4. hal_touch_init()     — 500 ms post-reset, CST9220
5. hal_imu_init()       — QMI8658 orientation (Wire partagé)
6. hal_audio_init()     — ES7210 + ES8311 + I2S RX/TX
7. ui_status_bar_init() — lv_layer_top (survit aux screen_load)
8. wifi_mgr_init()      — connect/retry/NTP
9. net_ota_init()
10. ble_mgr_init()      — provisioning WiFi + clés API
11. voice_engine_init() — FreeRTOS Core 0
12. reminders_app_init()— charge reminders.json + schedule wakeup
13. orchestrator_init()
14. ui_launcher_init()
```

---

## 10. Configuration build (Arduino IDE 3.3.8)

| Paramètre FQBN | Valeur |
|---------------|--------|
| Board | `esp32:esp32:esp32s3` |
| CDCOnBoot | `cdc` |
| FlashSize | `16M` |
| PartitionScheme | `custom` (partitions.csv) |
| PSRAM | `opi` |
| DebugLevel | `info` |
| EraseFlash | `all` |

### Librairies Arduino requises

| Librairie | Version min | Usage |
|-----------|-------------|-------|
| ArduinoJson | 7.x | Sérialisation reminders/config |
| LVGL | 9.x | Interface graphique |
| XPowersLib | 0.3.3 | AXP2101 PMU |
| Wire | core esp32 | I2C partagé |
| FFat | core esp32 | FATFS |
| Preferences | core esp32 | NVS |
| WiFiClientSecure | core esp32 | HTTPS Groq |

> **CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y** requis dans sdkconfig pour le bundle CA.
