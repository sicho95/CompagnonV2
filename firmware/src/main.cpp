/**
 * CompagnonV2 — main.cpp
 * OS Compagnon ESP32-S3 | Phase 1 — Squelette FreeRTOS
 *
 * Architecture dual-core :
 *   Core 0 : task_ble, task_network, task_voice_io
 *   Core 1 : task_ui_lvgl, task_os_main, task_agent_brain
 */

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

// ─── Modules internes (stubs pour phase 1) ──────────────────────────────
// Uncomment progressively as modules are implemented
// #include "hal/display.h"
// #include "hal/touch.h"
// #include "hal/pmu.h"
// #include "hal/audio_io.h"
// #include "hal/rtc.h"
// #include "hal/sd_card.h"
// #include "net/wifi_mgr.h"
// #include "net/ble_mgr.h"
// #include "system/os_main.h"
// #include "system/time_mgr.h"
// #include "ui/launcher.h"
// #include "ui/status_bar.h"
// #include "storage/nvs_mgr.h"
// #include "storage/fatfs_mgr.h"

// ─── Inter-task communication ───────────────────────────────────────
// Queues
QueueHandle_t g_queue_voice_text   = nullptr;  // voice_io  → os_main (STT result)
QueueHandle_t g_queue_tts_request  = nullptr;  // os_main   → voice_io (TTS text)
QueueHandle_t g_queue_ble_rx       = nullptr;  // ble       → os_main (inbound BLE cmds)
QueueHandle_t g_queue_ble_tx       = nullptr;  // os_main   → ble (outbound BLE events)

// Semaphores
SemaphoreHandle_t g_sem_lvgl       = nullptr;  // protects LVGL not-thread-safe calls
SemaphoreHandle_t g_sem_fatfs      = nullptr;  // protects FATFS concurrent access

// Flags (updated atomically)
volatile bool g_wifi_connected     = false;
volatile bool g_ble_connected      = false;
volatile bool g_silent_mode        = false;
volatile uint8_t g_battery_pct     = 100;
volatile bool g_charging           = false;

// ─── Task stack sizes ──────────────────────────────────────────
#define STACK_LVGL         (8  * 1024)
#define STACK_OS_MAIN      (8  * 1024)
#define STACK_BLE          (8  * 1024)
#define STACK_NETWORK      (6  * 1024)
#define STACK_VOICE_IO     (12 * 1024)  // larger : I2S DMA + wake word
#define STACK_AGENT_BRAIN  (16 * 1024)  // larger : JSON + LLM responses

// ─── Task priorities ───────────────────────────────────────────
#define PRI_LVGL        (configMAX_PRIORITIES - 1)  // highest on Core 1
#define PRI_VOICE_IO    (configMAX_PRIORITIES - 1)  // highest on Core 0
#define PRI_OS_MAIN     (tskIDLE_PRIORITY + 3)
#define PRI_BLE         (tskIDLE_PRIORITY + 3)
#define PRI_NETWORK     (tskIDLE_PRIORITY + 2)
#define PRI_AGENT_BRAIN (tskIDLE_PRIORITY + 1)

// ─── CORE 1 : LVGL task ───────────────────────────────────────
void task_ui_lvgl(void* arg) {
    Serial.println("[LVGL] task started on Core " + String(xPortGetCoreID()));

    // TODO Phase 1 Step 2:
    // hal_display_init();
    // hal_touch_init();
    // lv_init();
    // lv_disp_drv_register(...)
    // lv_indev_drv_register(...)
    // launcher_init();
    // status_bar_init();

    uint32_t last_tick = millis();
    for (;;) {
        uint32_t now = millis();
        uint32_t elapsed = now - last_tick;
        last_tick = now;

        if (xSemaphoreTake(g_sem_lvgl, pdMS_TO_TICKS(5)) == pdTRUE) {
            // lv_tick_inc(elapsed);
            // lv_timer_handler();
            xSemaphoreGive(g_sem_lvgl);
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// ─── CORE 1 : OS main task ────────────────────────────────────
void task_os_main(void* arg) {
    Serial.println("[OS] task started on Core " + String(xPortGetCoreID()));

    // TODO Phase 1 Step 2:
    // nvs_mgr_init();
    // fatfs_mgr_init();
    // sd_mgr_init();     // optional, non-blocking
    // hal_pmu_init();
    // hal_rtc_init();
    // time_mgr_init();
    // app_registry_init();  // register all 8 apps
    // app_orchestrator_init();
    // scheduler_init();  // cron jobs

    for (;;) {
        // hal_pmu_tick();           // update g_battery_pct, g_charging
        // wifi_mgr_tick();          // update g_wifi_connected
        // ble_mgr_tick();           // update g_ble_connected
        // status_bar_tick();        // update date/time display
        // agent_scheduler_tick();   // cron jobs
        // app_orchestrator_tick();  // manage app lifecycle
        // reminders_scheduler_tick(); // check upcoming reminders

        // Drain BLE RX queue
        // BleMessage msg;
        // while (xQueueReceive(g_queue_ble_rx, &msg, 0) == pdTRUE) {
        //     ble_protocol_handle(msg);
        // }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ─── CORE 0 : BLE task ─────────────────────────────────────────
void task_ble(void* arg) {
    Serial.println("[BLE] task started on Core " + String(xPortGetCoreID()));

    // TODO Phase 2:
    // ble_mgr_init();  // GATT server : 8 characteristics
    // ble_mgr_start_advertising();

    for (;;) {
        // ble_mgr_process(); // handle connect/disconnect/write events
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// ─── CORE 0 : Network (WiFi + NTP + OTA) ────────────────────────
void task_network(void* arg) {
    Serial.println("[NET] task started on Core " + String(xPortGetCoreID()));

    // TODO Phase 1 Step 2:
    // wifi_mgr_init();  // try saved creds (NVS), fallback AP if needed

    for (;;) {
        // wifi_mgr_tick();  // reconnect if needed
        // if (g_wifi_connected) {
        //     time_mgr_ntp_sync_if_needed();
        // }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ─── CORE 0 : Voice I/O (wake word + STT + TTS) ─────────────────
void task_voice_io(void* arg) {
    Serial.println("[VOICE] task started on Core " + String(xPortGetCoreID()));

    // TODO Phase 3:
    // hal_audio_io_init();
    // wake_word_init(WAKE_WORD);
    // asr_init();
    // tts_init();

    for (;;) {
        // uint8_t audio_buf[512];
        // if (hal_mic_read(audio_buf, 512)) {
        //     if (wake_word_detect(audio_buf, 512)) {
        //         power_mgr_wakeup_screen();
        //         char stt_result[256];
        //         if (asr_transcribe(stt_result, sizeof(stt_result))) {
        //             xQueueSend(g_queue_voice_text, stt_result, 0);
        //         }
        //     }
        // }
        // char tts_buf[512];
        // if (xQueueReceive(g_queue_tts_request, tts_buf, 0) == pdTRUE) {
        //     if (!g_silent_mode) tts_speak(tts_buf);
        // }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ─── CORE 1 : Agent brain (ReAct + memory + tools) ──────────────
void task_agent_brain(void* arg) {
    Serial.println("[BRAIN] task started on Core " + String(xPortGetCoreID()));

    // TODO Phase 3:
    // memory_l0_init();
    // memory_l2_load();  // MEMORY.md
    // agents_loader_init();
    // react_engine_init();

    for (;;) {
        // char voice_text[256];
        // if (xQueueReceive(g_queue_voice_text, voice_text, pdMS_TO_TICKS(100)) == pdTRUE) {
        //     react_engine_run(voice_text);
        // }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ─── setup() ──────────────────────────────────────────────
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

    // ——— Create inter-task queues ———
    g_queue_voice_text  = xQueueCreate(4,  256);   // 4 STT results
    g_queue_tts_request = xQueueCreate(8,  512);   // 8 TTS requests
    g_queue_ble_rx      = xQueueCreate(16, 256);   // 16 BLE messages
    g_queue_ble_tx      = xQueueCreate(16, 256);

    // ——— Create semaphores ———
    g_sem_lvgl  = xSemaphoreCreateMutex();
    g_sem_fatfs = xSemaphoreCreateMutex();

    Serial.println("[BOOT] Queues + semaphores created");

    // ——— Spawn tasks ———
    // Core 1 tasks
    xTaskCreatePinnedToCore(task_ui_lvgl,    "LVGL",   STACK_LVGL,        nullptr, PRI_LVGL,        nullptr, 1);
    xTaskCreatePinnedToCore(task_os_main,    "OS",     STACK_OS_MAIN,     nullptr, PRI_OS_MAIN,     nullptr, 1);
    xTaskCreatePinnedToCore(task_agent_brain,"BRAIN",  STACK_AGENT_BRAIN, nullptr, PRI_AGENT_BRAIN, nullptr, 1);

    // Core 0 tasks
    xTaskCreatePinnedToCore(task_ble,        "BLE",    STACK_BLE,         nullptr, PRI_BLE,         nullptr, 0);
    xTaskCreatePinnedToCore(task_network,    "NET",    STACK_NETWORK,     nullptr, PRI_NETWORK,     nullptr, 0);
    xTaskCreatePinnedToCore(task_voice_io,   "VOICE",  STACK_VOICE_IO,    nullptr, PRI_VOICE_IO,    nullptr, 0);

    Serial.println("[BOOT] All tasks spawned. OS running.");
}

// ─── loop() — not used (all logic in FreeRTOS tasks) ────────────
void loop() {
    // Deliberately empty.
    // FreeRTOS scheduler handles everything.
    vTaskDelay(pdMS_TO_TICKS(10000));
}
