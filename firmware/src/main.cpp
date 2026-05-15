/**
 * CompagnonV2 — main.cpp
 * OS Compagnon ESP32-S3 | Phase 1 — Squelette FreeRTOS
 *
 * Architecture dual-core :
 *   Core 0 : task_ble, task_network, task_voice_io
 *   Core 1 : task_ui_lvgl, task_os_main, task_agent_brain
 *
 * LVGL 9 — API de référence :
 *   lv_display_create()      (remplace lv_disp_drv_register)
 *   lv_indev_create()        (remplace lv_indev_drv_register)
 *   lv_indev_set_read_cb()   (remplace lv_indev_drv_t.read_cb)
 *   lv_draw_buf_t            (remplace lv_disp_draw_buf_t)
 */

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

// ─── Modules internes (décommentés progressivement) ─────────────────────
// #include "hal/display.h"
// #include "hal/touch.h"
// #include "hal/pmu.h"
// #include "hal/audio_io.h"
// #include "hal/rtc.h"
// #include "net/wifi_mgr.h"
// #include "net/ble_mgr.h"
// #include "system/os_main.h"
// #include "system/time_mgr.h"
// #include "ui/launcher.h"
// #include "ui/status_bar.h"
// #include "storage/nvs_mgr.h"
// #include "storage/fatfs_mgr.h"

// ─── Inter-task communication ────────────────────────────────────────────
QueueHandle_t g_queue_voice_text   = nullptr;  // voice_io  → os_main (STT result)
QueueHandle_t g_queue_tts_request  = nullptr;  // os_main   → voice_io (TTS text)
QueueHandle_t g_queue_ble_rx       = nullptr;  // ble       → os_main (inbound BLE cmds)
QueueHandle_t g_queue_ble_tx       = nullptr;  // os_main   → ble (outbound BLE events)

SemaphoreHandle_t g_sem_lvgl       = nullptr;  // protège les appels LVGL (non thread-safe)
SemaphoreHandle_t g_sem_fatfs      = nullptr;  // protège l'accès concurrent FATFS

// Flags globaux mis à jour atomiquement
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
#define STACK_VOICE_IO     (12 * 1024)  // I2S DMA + wake word
#define STACK_AGENT_BRAIN  (16 * 1024)  // JSON + LLM responses

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

    // TODO Step 3 — init LVGL 9 :
    // Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
    // hal_display_init();          // lv_init() + lv_display_create() + lv_draw_buf_t
    // touch_init();                // CST9220
    // lv_indev_t *indev = lv_indev_create();
    // lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    // lv_indev_set_read_cb(indev, touch_read_cb);
    // status_bar_init();
    // launcher_init();

    uint32_t last_tick = millis();
    for (;;) {
        uint32_t now     = millis();
        uint32_t elapsed = now - last_tick;
        last_tick        = now;

        if (xSemaphoreTake(g_sem_lvgl, pdMS_TO_TICKS(5)) == pdTRUE) {
            // lv_tick_inc(elapsed);
            // lv_timer_handler();
            xSemaphoreGive(g_sem_lvgl);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// ─── CORE 1 : OS main task ───────────────────────────────────────────────
void task_os_main(void* arg) {
    Serial.println("[OS] task started on Core " + String(xPortGetCoreID()));

    // TODO Step 5 :
    // nvs_mgr_init();
    // fatfs_mgr_init();
    // hal_pmu_init();
    // hal_rtc_init();
    // time_mgr_init();
    // app_registry_init();       // enregistre les 8 apps
    // scheduler_init();          // cron jobs (Jardinier, Rappels...)

    for (;;) {
        // hal_pmu_tick();              // màj g_battery_pct, g_charging
        // wifi_mgr_tick();             // màj g_wifi_connected
        // ble_mgr_tick();              // màj g_ble_connected
        // status_bar_tick();           // màj date/heure
        // agent_scheduler_tick();
        // reminders_scheduler_tick();
        // BleMessage msg;
        // while (xQueueReceive(g_queue_ble_rx, &msg, 0) == pdTRUE)
        //     ble_protocol_handle(msg);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ─── CORE 0 : BLE task ───────────────────────────────────────────────────
void task_ble(void* arg) {
    Serial.println("[BLE] task started on Core " + String(xPortGetCoreID()));
    // TODO Step 7 : ble_mgr_init() + ble_mgr_start_advertising()
    for (;;) {
        // ble_mgr_process();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// ─── CORE 0 : Network (WiFi + NTP + OTA) ────────────────────────────────
void task_network(void* arg) {
    Serial.println("[NET] task started on Core " + String(xPortGetCoreID()));
    // TODO Step 6 : wifi_mgr_init()
    for (;;) {
        // wifi_mgr_tick();
        // if (g_wifi_connected) time_mgr_ntp_sync_if_needed();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ─── CORE 0 : Voice I/O (wake word + STT + TTS) ─────────────────────────
void task_voice_io(void* arg) {
    Serial.println("[VOICE] task started on Core " + String(xPortGetCoreID()));
    // TODO Step 10 : hal_audio_io_init() + wake_word_init() + asr_init() + tts_init()
    for (;;) {
        // Pipeline : mic I2S → wake word → STT → xQueueSend(g_queue_voice_text)
        // + xQueueReceive(g_queue_tts_request) → tts_speak() si !g_silent_mode
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ─── CORE 1 : Agent brain (ReAct + memory + tools) ──────────────────────
void task_agent_brain(void* arg) {
    Serial.println("[BRAIN] task started on Core " + String(xPortGetCoreID()));
    // TODO Step 11 : memory_l0_init() + agents_loader_init() + react_engine_init()
    for (;;) {
        // char voice_text[256];
        // if (xQueueReceive(g_queue_voice_text, voice_text, pdMS_TO_TICKS(100)) == pdTRUE)
        //     react_engine_run(voice_text);
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
    Serial.printf(" Core count   : %d\n", ESP.getChipCores());
    Serial.printf(" Free heap    : %d bytes\n", ESP.getFreeHeap());
    Serial.printf(" PSRAM size   : %d bytes\n", ESP.getPsramSize());
    Serial.printf(" Flash size   : %d bytes\n", ESP.getFlashChipSize());
    Serial.println("------------------------------------");

    // Queues inter-tâches
    g_queue_voice_text  = xQueueCreate(4,  256);
    g_queue_tts_request = xQueueCreate(8,  512);
    g_queue_ble_rx      = xQueueCreate(16, 256);
    g_queue_ble_tx      = xQueueCreate(16, 256);

    // Sémaphores
    g_sem_lvgl  = xSemaphoreCreateMutex();
    g_sem_fatfs = xSemaphoreCreateMutex();

    Serial.println("[BOOT] Queues + semaphores created");

    // Spawn tâches — Core 1
    xTaskCreatePinnedToCore(task_ui_lvgl,     "LVGL",  STACK_LVGL,        nullptr, PRI_LVGL,        nullptr, 1);
    xTaskCreatePinnedToCore(task_os_main,     "OS",    STACK_OS_MAIN,     nullptr, PRI_OS_MAIN,     nullptr, 1);
    xTaskCreatePinnedToCore(task_agent_brain, "BRAIN", STACK_AGENT_BRAIN, nullptr, PRI_AGENT_BRAIN, nullptr, 1);

    // Spawn tâches — Core 0
    xTaskCreatePinnedToCore(task_ble,         "BLE",   STACK_BLE,         nullptr, PRI_BLE,         nullptr, 0);
    xTaskCreatePinnedToCore(task_network,     "NET",   STACK_NETWORK,     nullptr, PRI_NETWORK,     nullptr, 0);
    xTaskCreatePinnedToCore(task_voice_io,    "VOICE", STACK_VOICE_IO,    nullptr, PRI_VOICE_IO,    nullptr, 0);

    Serial.println("[BOOT] All tasks spawned. OS running.");
}

// ─── loop() — FreeRTOS gère tout, loop() inutilisé ───────────────────────
void loop() {
    vTaskDelay(pdMS_TO_TICKS(10000));
}
