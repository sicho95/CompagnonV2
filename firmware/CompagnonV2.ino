/*
 * CompagnonV2 — ESP32-S3 Waveshare AMOLED 2.16"
 * Framework : Arduino 3.3.8 / arduino-esp32
 * LVGL      : 9.x  (lv_init() OBLIGATOIRE avant tout appel LVGL)
 * Board     : Waveshare ESP32-S3-Touch-AMOLED-2.16
 *
 * Ordre d'init critique :
 *  0. lv_init()          — une seule fois, avant tout LVGL
 *  1. PMU                — rails ALDO1/ALDO3, bouton power
 *  2. NVS                — namespace "compagnon"
 *  3. Display            — lv_display_create() + buffers PSRAM
 *  4. Touch              — 500 ms post-reset, CST9220
 *  5. IMU                — QMI8658
 *  6. Audio              — I2S mic + codec
 *  7. apps_register_all  — descripteurs statiques (RAM ~0)
 *  8. os::os_start()     — lance toutes les tâches FreeRTOS
 *                          (ui_lvgl, os_main, voice_io, ble, network)
 *                          task_ui_lvgl appelle ui_status_bar_init()
 *                          + ui_launcher_init() en interne.
 *  9. Callback PMU power → menu UI
 *
 * NOTE LVGL tick :
 *  LV_TICK_CUSTOM 1 dans lv_conf.h → LVGL lit millis() directement.
 *  Ne PAS appeler lv_tick_inc() — double-comptage qui bloque le rendu.
 *  lv_timer_handler() est appelé dans task_ui_lvgl (FreeRTOS), pas ici.
 */

#include <lvgl.h>

#include "src/hal/display.h"
#include "src/hal/touch.h"
#include "src/hal/imu.h"
#include "src/hal/pmu.h"
#include "src/hal/audio.h"
#include "src/system/os_main.h"
#include "src/system/os_kernel.h"
#include "src/system/power_mgr.h"
#include "src/config/nvs_config.h"
#include "src/ui/status_bar.h"
#include <ArduinoJson.h>
#include <WiFi.h>

// ─── Setup ─────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[BOOT] CompagnonV2 — demarrage");

    // 0. LVGL — une seule fois, avant tout appel LVGL
    lv_init();
    Serial.println("[BOOT] LVGL init OK");

    // 1-6. Hardware
    hal_pmu_init();
    nvs_config_init();
    hal_display_init();   // lv_display_create() + flush_cb + buffers
    hal_touch_init();
    hal_imu_init();
    hal_audio_init();

    // 7. Enregistrer les apps dans le kernel (coût RAM ~0, pas de tâche)
    os::apps_register_all();

    // 8. Lancer toutes les tâches FreeRTOS
    //    task_ui_lvgl  → status_bar + launcher carousel + lv_timer_handler
    //    task_os_main  → kernel_init + kernel_tick (alarmes, intents vocaux)
    //    task_voice_io → wake word (Core 0)
    //    task_ble      → BLE (Core 0)
    //    task_network  → WiFi + NTP (Core 0)
    os::os_start();

    // 9. Callback long-press PMU → menu power
    hal_pmu_set_long_press_cb(ui_power_menu_show);

    Serial.println("[BOOT] Pret.");
}

// ─── Loop (Core 1) ─────────────────────────────────────────────────────────
// lv_timer_handler() est dans task_ui_lvgl (prio 5).
// Les tâches réseau/BLE/voice tournent en FreeRTOS — rien à faire ici.
// On garde seulement PMU (IRQ) et power_mgr (deep-sleep watchdog).
void loop() {
    hal_pmu_tick();
    power_mgr_tick();
    delay(20);
}
