// ============================================================
// CompagnonV2 — main.cpp
// Boot sequence :
//   1. SYS_PWR HIGH (power latch)
//   2. I2C + RTC PCF85063 → settimeofday
//   3. FFat + ReminderStore
//   4. Provisioning NVS depuis secrets.h (dév uniquement)
//   5. Display → Touch → PMU → Audio
//   6. Apps + FreeRTOS
// Wakeup deep sleep → wifi_init() → Scheduler::onWakeup() → re-sleep
// ============================================================
#include <Arduino.h>
#include "../../include/pins.h"
#include "../../include/secrets.h"
#include "hal/rtc.h"
#include "hal/display.h"
#include "hal/touch.h"
#include "hal/pmu.h"
#include "hal/hal_audio.h"
#include "system/os_kernel.h"
#include "system/os_main.h"
#include "system/scheduler.h"
#include "storage/reminder_store.h"
#include "storage/nvs_store.h"
#include "net/wifi_mgr.h"
#include <FFat.h>
#include <esp_sleep.h>

// ── Provisioning NVS depuis secrets.h (dév) ──────────────────────────────────
// En production ces valeurs arrivent via BLE (PWA → _ble_on_agent).
// On écrit dans NVS seulement si la clé est encore vide → la PWA peut
// écraser sans que le firmware réinitialise au redémarrage.
static void _provision_from_secrets() {

    // WiFi ──────────────────────────────────────────────────────────────────
#if defined(SECRET_WIFI_SSID)
    if (NvsStore::getString("wifi", "ssid").isEmpty()) {
        NvsStore::setString("wifi", "ssid",    SECRET_WIFI_SSID);
        NvsStore::setString("wifi", "pass",    SECRET_WIFI_PASS);
        NvsStore::setString("wifi", "ap_ssid", SECRET_AP_SSID);
        NvsStore::setString("wifi", "ap_pass", SECRET_AP_PASS);
        Serial.println("[BOOT] WiFi credentials → NVS");
    }
#endif

    // Groq ──────────────────────────────────────────────────────────────────
#if defined(SECRET_GROQ_API_KEY)
    if (NvsStore::getString("app", "groq_api_key").isEmpty()) {
        NvsStore::setString("app", "groq_api_key", SECRET_GROQ_API_KEY);
        Serial.println("[BOOT] Groq API key → NVS");
    }
#endif

    // Twelve Data (bourse) ──────────────────────────────────────────────────
#if defined(SECRET_TWELVE_DATA_KEY)
    if (NvsStore::getString("bourse", "td_api_key").isEmpty()) {
        NvsStore::setString("bourse", "td_api_key", SECRET_TWELVE_DATA_KEY);
        Serial.println("[BOOT] Twelve Data key → NVS");
    }
#endif

    // WeatherAPI (météo) ────────────────────────────────────────────────────
#if defined(SECRET_WEATHER_API_KEY)
    if (NvsStore::getString("meteo", "weather_api_key").isEmpty()) {
        NvsStore::setString("meteo", "weather_api_key", SECRET_WEATHER_API_KEY);
        Serial.println("[BOOT] WeatherAPI key → NVS");
    }
#endif

    // Tuya ──────────────────────────────────────────────────────────────────
#if defined(SECRET_TUYA_ACCESS_ID)
    if (NvsStore::getString("tuya", "access_id").isEmpty()) {
        NvsStore::setString("tuya", "access_id",  SECRET_TUYA_ACCESS_ID);
        NvsStore::setString("tuya", "access_key", SECRET_TUYA_ACCESS_KEY);
        NvsStore::setString("tuya", "region",     SECRET_TUYA_REGION);
        Serial.println("[BOOT] Tuya credentials → NVS");
    }
#endif

    // Ecovacs ───────────────────────────────────────────────────────────────
#if defined(SECRET_ECOVACS_ACCOUNT)
    if (NvsStore::getString("ecovacs", "account").isEmpty()) {
        NvsStore::setString("ecovacs", "account",   SECRET_ECOVACS_ACCOUNT);
        NvsStore::setString("ecovacs", "password",  SECRET_ECOVACS_PASSWORD);
        NvsStore::setString("ecovacs", "continent", SECRET_ECOVACS_CONTINENT);
        Serial.println("[BOOT] Ecovacs credentials → NVS");
    }
#endif
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n=== CompagnonV2 boot ===");

    // 1. Power latch
    pinMode(PIN_SYS_PWR, OUTPUT);
    digitalWrite(PIN_SYS_PWR, HIGH);

    // 2. I2C + RTC
    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
    Wire.setClock(400000);
    if (hal::rtc_init()) {
        hal::rtc_apply_to_system();
    } else {
        Serial.println("[BOOT] WARNING: RTC not available");
    }

    // 3. FATFS + ReminderStore
    if (!FFat.begin(true)) {
        Serial.println("[BOOT] FFat mount failed — reminders unavailable");
    } else {
        ReminderStore::init();
    }

    // 4. Provisioning dév (no-op en prod si NVS déjà rempli par BLE)
    _provision_from_secrets();

    // ── Wakeup depuis deep sleep ─────────────────────────────────────────────
    // FIX-ORANGE-6 : WiFi DOIT être connecté avant d'appeler
    // Scheduler::onWakeup() qui fait une requête TTS Groq.
    // On initialise le WiFi ici (bloquant), puis on redort après TTS.
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_TIMER || cause == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("[BOOT] Wakeup from sleep → wifi_init + Scheduler::onWakeup()");
        hal::audio_init();
        net::wifi_init();          // connexion STA bloquante (retry × 10 × 1s)
        Scheduler::onWakeup();     // TTS rappel → audio_play_pcm → re-sleep interne
        esp_deep_sleep_start();
    }

    // 5. Périphériques
    hal::display_init();
    hal::touch_init();
    hal::pmu_init();
    hal::audio_init();

    // 6. Apps + FreeRTOS
    // FIX-ROUGE-1 : apps_register_all() supprimé ici — appelé dans
    // task_os_main (os_main.cpp) pour éviter double initialisation du registre.
    os::os_start();
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
