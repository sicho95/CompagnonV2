// ============================================================
// CompagnonV2 — main.cpp
// Boot sequence :
//   1. SYS_PWR HIGH (power latch)
//   2. I2C + RTC PCF85063 → settimeofday
//   3. FFat + ReminderStore
//   4. Provisioning NVS depuis secrets.h (dév uniquement)
//   5. Display → Touch → PMU → Audio
//   6. Apps + FreeRTOS
// Wakeup deep sleep → Scheduler::onWakeup() avant boot UI
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
#include <FFat.h>
#include <esp_sleep.h>

// ── Provisioning NVS depuis secrets.h (dév) ──────────────────
// En production ces valeurs arrivent via BLE depuis la PWA.
// En dév on écrit seulement si la clé NVS est encore vide.
static void _provision_from_secrets() {
#ifdef SECRET_WIFI_SSID
    if (NvsStore::getString("wifi", "ssid").isEmpty()) {
        NvsStore::setString("wifi", "ssid",    SECRET_WIFI_SSID);
        NvsStore::setString("wifi", "pass",    SECRET_WIFI_PASS);
        NvsStore::setString("wifi", "ap_ssid", SECRET_AP_SSID);
        NvsStore::setString("wifi", "ap_pass", SECRET_AP_PASS);
        Serial.println("[BOOT] WiFi credentials written to NVS from secrets.h");
    }
#endif
#ifdef SECRET_GROQ_API_KEY
    if (NvsStore::getString("app", "groq_api_key").isEmpty()) {
        NvsStore::setString("app", "groq_api_key", SECRET_GROQ_API_KEY);
        Serial.println("[BOOT] Groq API key written to NVS from secrets.h");
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

    // Wakeup depuis deep sleep → traiter rappel AVANT init UI
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_TIMER || cause == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("[BOOT] Wakeup from sleep → Scheduler::onWakeup()");
        hal::audio_init();
        Scheduler::onWakeup();
        esp_deep_sleep_start();
    }

    // 5. Périphériques
    hal::display_init();
    hal::touch_init();
    hal::pmu_init();
    hal::audio_init();

    // 6. Apps + FreeRTOS
    os::apps_register_all();
    os::os_start();
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
