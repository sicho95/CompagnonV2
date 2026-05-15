// ============================================================
// CompagnonV2 — main.cpp
// Boot sequence stricte :
//   1. SYS_PWR HIGH (power latch)
//   2. I2C + RTC PCF85063 → settimeofday (offline time)
//   3. Display → Touch → PMU → Audio
//   4. Enregistrement des apps (coût RAM ~0)
//   5. Lancement tâches FreeRTOS dual-core
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

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n=== CompagnonV2 boot ===");

    // 1. Power latch — maintenir l'alimentation
    pinMode(PIN_SYS_PWR, OUTPUT);
    digitalWrite(PIN_SYS_PWR, HIGH);

    // 2. I2C + RTC — PRIORITÉ ABSOLUE pour avoir l'heure offline
    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
    Wire.setClock(400000);
    if (hal::rtc_init()) {
        hal::rtc_apply_to_system(); // PCF85063 → settimeofday
    } else {
        Serial.println("[BOOT] WARNING: RTC not available, time unknown");
    }

    // 3. Périphériques
    hal::display_init();
    hal::touch_init();
    hal::pmu_init();
    hal::audio_init();

    // 4. Enregistrement des apps (statique, pas d'allocation LVGL)
    os::apps_register_all();

    // 5. Lancement kernel FreeRTOS
    os::os_start();
}

void loop() {
    vTaskDelay(portMAX_DELAY); // FreeRTOS prend la main
}
