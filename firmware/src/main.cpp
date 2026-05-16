// ============================================================
// CompagnonV2 — main.cpp
// Boot sequence :
//   1. SYS_PWR HIGH (power latch)
//   2. I2C + RTC PCF85063 → settimeofday (offline time)
//   3. FFat mount → ReminderStore init
//   4. Display → Touch → PMU → Audio
//   5. Enregistrement des apps
//   6. Lancement tâches FreeRTOS dual-core
// Wakeup deep sleep → Scheduler::onWakeup() avant boot complet
// ============================================================
#include <Arduino.h>
#include "../../include/pins.h"
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
        Serial.println("[BOOT] ReminderStore loaded");
    }

    // Wakeup depuis deep sleep → traiter rappel AVANT init UI
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_TIMER || cause == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("[BOOT] Wakeup from sleep → Scheduler::onWakeup()");
        // Audio init minimal pour TTS
        hal::audio_init();
        Scheduler::onWakeup();
        // Retour en sleep si pas d'interaction
        esp_deep_sleep_start();
        // (ne revient pas)
    }

    // 4. Périphériques
    hal::display_init();
    hal::touch_init();
    hal::pmu_init();
    hal::audio_init();

    // 5. Enregistrement des apps
    os::apps_register_all();

    // 6. Kernel FreeRTOS
    os::os_start();
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
