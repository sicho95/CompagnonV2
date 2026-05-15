// ============================================================
// CompagnonV2 — system/os_main.cpp
// Lancement tâches FreeRTOS dual-core
//
// Core 0 : task_voice_io, task_ble, task_network
// Core 1 : task_ui_lvgl, task_os_main
// ============================================================
#include "os_main.h"
#include "os_kernel.h"
#include "../hal/rtc.h"
#include "../ui/status_bar.h"
#include "../ui/launcher.h"
#include "../voice/voice_engine.h"
#include "../net/wifi_mgr.h"
#include "../net/ble_mgr.h"
#include <lvgl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Arduino.h>

namespace os {

// ── Handles tâches ───────────────────────────────────────────
static TaskHandle_t _h_ui      = nullptr;
static TaskHandle_t _h_os      = nullptr;
static TaskHandle_t _h_voice   = nullptr;
static TaskHandle_t _h_ble     = nullptr;
static TaskHandle_t _h_net     = nullptr;

// ── task_ui_lvgl — Core 1, prio 5 ────────────────────────────
static void task_ui_lvgl(void*) {
    // Init LVGL
    lv_init();
    ui::status_bar_init();
    ui::launcher_init();
    Serial.println("[UI] LVGL ready");
    for (;;) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// ── task_os_main — Core 1, prio 3 ────────────────────────────
static void task_os_main(void*) {
    kernel_init();
    uint32_t last_rtc_check = 0;
    for (;;) {
        uint32_t now = millis();
        // Tick kernel (traite intents vocaux en attente)
        kernel_tick();
        // Update status bar toutes les secondes
        if (now - last_rtc_check >= 1000) {
            last_rtc_check = now;
            ui::status_bar_tick();
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// ── task_voice_io — Core 0, prio 6 ───────────────────────────
static void task_voice_io(void*) {
    voice::voice_engine_init();
    Serial.println("[VOICE] task started on Core 0");
    voice::voice_engine_run(); // boucle infinie interne
    vTaskDelete(nullptr);
}

// ── task_ble — Core 0, prio 4 ────────────────────────────────
static void task_ble(void*) {
    net::ble_init();
    Serial.println("[BLE] task started");
    for (;;) {
        net::ble_tick();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ── task_network — Core 0, prio 3 ────────────────────────────
static void task_network(void*) {
    net::wifi_init();
    Serial.println("[NET] task started");
    bool ntp_done = false;
    for (;;) {
        net::wifi_tick();
        // Sync NTP → RTC dès que WiFi connecté (une seule fois par boot)
        if (!ntp_done && net::wifi_is_connected()) {
            time_t epoch = net::wifi_get_ntp_epoch(); // bloquant ~500ms
            if (epoch > 0) {
                hal::rtc_sync_from_ntp(epoch);
                os::kernel_set_time_valid(true);
                os::kernel_schedule_next_reminder();
                ntp_done = true;
                Serial.println("[NET] NTP sync done → RTC updated");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ── os_start — appelé depuis setup() ─────────────────────────
void os_start() {
    Serial.println("[OS] Starting FreeRTOS tasks...");
    // Core 1
    xTaskCreatePinnedToCore(task_ui_lvgl,  "ui_lvgl",  8192, nullptr, 5, &_h_ui,    1);
    xTaskCreatePinnedToCore(task_os_main,  "os_main",  4096, nullptr, 3, &_h_os,    1);
    // Core 0
    xTaskCreatePinnedToCore(task_voice_io, "voice_io", 8192, nullptr, 6, &_h_voice, 0);
    xTaskCreatePinnedToCore(task_ble,      "ble",      6144, nullptr, 4, &_h_ble,   0);
    xTaskCreatePinnedToCore(task_network,  "network",  6144, nullptr, 3, &_h_net,   0);
    Serial.println("[OS] All tasks launched");
}

} // namespace os
