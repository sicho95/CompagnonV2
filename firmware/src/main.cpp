// ============================================================
// CompagnonV2 — main.cpp
// Boot sequence :
//   1. SYS_PWR HIGH (power latch)
//   2. I2C + RTC PCF85063 → settimeofday (offline time)
//   3. FFat mount → ReminderStore::load()
//   4. Display → Touch → PMU → Audio HAL
//   5. Enregistrement apps (une seule fois ici — R2)
//   6. Lancement tâches FreeRTOS dual-core
// ============================================================
#include <Arduino.h>
#include <FFat.h>
#include "../../include/pins.h"
#include "hal/rtc.h"
#include "hal/display.h"
#include "hal/touch.h"
#include "hal/pmu.h"
#include "hal/hal_audio.h"
#include "storage/reminder_store.h"
#include "system/os_kernel.h"
#include "system/os_main.h"

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n=== CompagnonV2 boot ===");

    // 1. Power latch
    pinMode(PIN_SYS_PWR, OUTPUT);
    digitalWrite(PIN_SYS_PWR, HIGH);

    // 2. I2C + RTC — heure offline prioritaire
    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
    Wire.setClock(400000);
    if (hal::rtc_init()) {
        hal::rtc_apply_to_system();
    } else {
        Serial.println("[BOOT] WARNING: RTC not available, time unknown");
    }

    // 3. FATFS + chargement des rappels persistés
    if (!FFat.begin(true)) {
        Serial.println("[BOOT] WARNING: FFat mount failed — reminders unavailable");
    } else {
        ReminderStore::load();
        Serial.printf("[BOOT] ReminderStore: %u reminders loaded\n",
                      (unsigned)ReminderStore::getAll().size());
    }

    // 4. Périphériques
    hal::display_init();
    hal::touch_init();
    hal::pmu_init();
    hal::audio_init();

    // 5. R2 — enregistrement apps UNE seule fois (task_os_main ne le refait pas)
    os::apps_register_all();

    // 6. Kernel + tâches FreeRTOS
    os::os_start();
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
