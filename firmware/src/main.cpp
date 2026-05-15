/**
 * CompagnonV2 — main.cpp
 * OS Compagnon ESP32-S3 | Phase 1 Steps 1-3
 *
 * Architecture dual-core :
 *   Core 0 : task_ble, task_network, task_voice_io
 *   Core 1 : task_ui_lvgl, task_os_main, task_agent_brain
 *
 * LVGL 9 API : lv_display_create(), lv_indev_create(), lv_indev_set_read_cb()
 */

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

// ─── HAL + UI (décommentés progressivement) ─────────────────────────────
// #include "hal/display.h"
// #include "hal/touch.h"
// #include "hal/pmu.h"
// #include "hal/audio_io.h"
// #include "hal/rtc.h"
// #include "net/wifi_mgr.h"
// #include "net/ble_mgr.h"
// #include "system/os_main.h"
// #include "system/time_mgr.h"
#include "ui/status_bar.h"   // Step 3 : status bar LVGL 9
// #include "ui/launcher.h"  // Step 4
// #include "storage/nvs_mgr.h"
// #include "storage/fatfs_mgr.h"

// ─── Inter-task communication ────────────────────────────────────────────
QueueHandle_t g_queue_voice_text   = nullptr;
QueueHandle_t g_queue_tts_request  = nullptr;
QueueHandle_t g_queue_ble_rx       = nullptr;
QueueHandle_t g_queue_ble_tx       = nullptr;

SemaphoreHandle_t g_sem_lvgl       = nullptr;
SemaphoreHandle_t g_sem_fatfs      = nullptr;

volatile bool    g_wifi_connected  = false;
volatile bool    g_ble_connected   = false;
volatile bool    g_silent_mode     = false;
volatile uint8_t g_battery_pct     = 100;
volatile bool    g_charging        = false;

// ─── Task stack sizes ────────────────────────────────────────────────────
#define STACK_LVGL         (8  * 1024)
#define STACK_OS_MAIN      (8  * 1024)
#define STACK_BLE          (8  * 1024)
#define STACK_NETWORK      (6  * 1024)
#define STACK_VOICE_IO     (12 * 1024)
#define STACK_AGENT_BRAIN  (16 * 1024)

// ─── Task priorities ─────────────────────────────────────────────────────
#define PRI_LVGL        (configMAX_PRIORITIES - 1)
#define PRI_VOICE_IO    (configMAX_PRIORITIES - 1)
#define PRI_OS_MAIN     (tskIDLE_PRIORITY + 3)
#define PRI_BLE         (tskIDLE_PRIORITY + 3)
#define PRI_NETWORK     (tskIDLE_PRIORITY + 2)
#define PRI_AGENT_BRAIN (tskIDLE_PRIORITY + 1)

// ─── CORE 1 : LVGL task ──────────────────────────────────────────────────
void task_ui_lvgl(void* arg) {
    Serial.println("[LVGL] task started on Core " + String(xPortGetCoreID()));

    // TODO Step 4 (display + touch init complets) :
    // hal_display_init();         // CO5300 QSPI via Arduino_GFX
    // hal_touch_init();           // CST9220
    // lv_init();
    // lv_display_t *disp = lv_display_create(480, 480); // LVGL 9 API
    // lv_display_set_flush_cb(disp, display_flush_cb);
    // lv_display_set_draw_buffers(disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    // lv_indev_t *indev = lv_indev_create();             // LVGL 9 API
    // lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    // lv_indev_set_read_cb(indev, touch_read_cb);        // CST9220 callback
    // status_bar_init();
    // launcher_init();

    uint32_t last_tick = millis();
    for (;;) {
        if (xSemaphoreTake(g_sem_lvgl, pdMS_TO_TICKS(5)) == pdTRUE) {
            uint32_t now = millis();
            lv_tick_inc(now - last_tick);  // LVGL 9 : même API
            last_tick = now;
            lv_timer_handler();            // LVGL 9 : même API
            xSemaphoreGive(g_sem_lvgl);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// ─── CORE 1 : OS main task ───────────────────────────────────────────────
void task_os_main(void* arg) {
    Serial.println("[OS] task started on Core " + String(xPortGetCoreID()));

    // TODO Steps suivants :
    // nvs_mgr_init();
    // fatfs_mgr_init();
    // hal_pmu_init();
    // hal_rtc_init();
    // time_mgr_init();
    // app_registry_init();
    // scheduler_init();

    for (;;) {
        // hal_pmu_tick();              // g_battery_pct, g_charging
        // wifi_mgr_tick();             // g_wifi_connected
        // ble_mgr_tick();              // g_ble_connected
        // status_bar_tick();           // refresh date/heure + icônes + batterie
        // agent_scheduler_tick();
        // app_orchestrator_tick();
        // reminders_scheduler_tick();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ─── CORE 0 : BLE task ───────────────────────────────────────────────────
void task_ble(void* arg) {
    Serial.println("[BLE] task started on Core " + String(xPortGetCoreID()));
    // TODO Phase 2 Step 7 : ble_mgr_init() + 8 caractéristiques
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// ─── CORE 0 : Network ────────────────────────────────────────────────────
void task_network(void* arg) {
    Serial.println("[NET] task started on Core " + String(xPortGetCoreID()));
    // TODO Step 6 : wifi_mgr_init()
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ─── CORE 0 : Voice I/O ──────────────────────────────────────────────────
void task_voice_io(void* arg) {
    Serial.println("[VOICE] task started on Core " + String(xPortGetCoreID()));
    // TODO Phase 3 Step 10 : wake word + STT + TTS
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ─── CORE 1 : Agent brain ────────────────────────────────────────────────
void task_agent_brain(void* arg) {
    Serial.println("[BRAIN] task started on Core " + String(xPortGetCoreID()));
    // TODO Phase 3 Step 11 : mimiclaw ReAct engine
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ─── setup() ─────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=====================================");
    Serial.println(" CompagnonV2 OS — booting...");
    Serial.println("=====================================");
    Serial.printf(" Core count : %d\n",  ESP.getChipCores());
    Serial.printf(" Free heap  : %d\n",  ESP.getFreeHeap());
    Serial.printf(" PSRAM size : %d\n",  ESP.getPsramSize());
    Serial.printf(" Flash size : %d\n",  ESP.getFlashChipSize());
    Serial.println("-------------------------------------");

    g_queue_voice_text  = xQueueCreate(4,  256);
    g_queue_tts_request = xQueueCreate(8,  512);
    g_queue_ble_rx      = xQueueCreate(16, 256);
    g_queue_ble_tx      = xQueueCreate(16, 256);
    g_sem_lvgl          = xSemaphoreCreateMutex();
    g_sem_fatfs         = xSemaphoreCreateMutex();

    Serial.println("[BOOT] Queues + semaphores OK");

    xTaskCreatePinnedToCore(task_ui_lvgl,    "LVGL",  STACK_LVGL,        nullptr, PRI_LVGL,        nullptr, 1);
    xTaskCreatePinnedToCore(task_os_main,    "OS",    STACK_OS_MAIN,     nullptr, PRI_OS_MAIN,     nullptr, 1);
    xTaskCreatePinnedToCore(task_agent_brain,"BRAIN", STACK_AGENT_BRAIN, nullptr, PRI_AGENT_BRAIN, nullptr, 1);
    xTaskCreatePinnedToCore(task_ble,        "BLE",   STACK_BLE,         nullptr, PRI_BLE,         nullptr, 0);
    xTaskCreatePinnedToCore(task_network,    "NET",   STACK_NETWORK,     nullptr, PRI_NETWORK,     nullptr, 0);
    xTaskCreatePinnedToCore(task_voice_io,   "VOICE", STACK_VOICE_IO,    nullptr, PRI_VOICE_IO,    nullptr, 0);

    Serial.println("[BOOT] All tasks spawned. OS running.");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(10000));
}
