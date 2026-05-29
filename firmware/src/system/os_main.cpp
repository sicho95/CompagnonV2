// ============================================================
// CompagnonV2 — system/os_main.cpp
// fix WiFi OOM : vTaskDelay 5s + esp_wifi_set_ps(WIFI_PS_NONE)
//   BLE et WiFi partagent les buffers DMA internes ESP32-S3.
//   Initialiser les deux simultanément → ESP_ERR_NO_MEM (0x101).
//   Solution : attendre 5 s que BLE stabilise son heap, puis
//   désactiver le power-save WiFi (WIFI_PS_NONE) pour éviter
//   les conflits de scheduling DMA.
// ============================================================
#include "os_main.h"
#include "os_kernel.h"
#include "../hal/rtc.h"
#include "../ui/status_bar.h"
#include "../ui/launcher.h"
#include "../ui/notification_mgr.h"
#include "../net/wifi_mgr.h"
#include "../net/ble_manager.h"
#include "../storage/reminder_store.h"
#include "../system/scheduler.h"
#include "../voice/voice_engine.h"
#include <lvgl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_wifi.h>
#include <Arduino.h>

namespace os {

static TaskHandle_t _h_ui    = nullptr;
static TaskHandle_t _h_os    = nullptr;
static TaskHandle_t _h_voice = nullptr;
static TaskHandle_t _h_ble   = nullptr;
static TaskHandle_t _h_net   = nullptr;

// ── task_ui_lvgl — Core 1, prio 5 ────────────────────────────
static void task_ui_lvgl(void*) {
    ui_status_bar_init();
    ui_launcher_init();  // appelle ui_status_bar_raise() en interne
    Serial.println("[UI] LVGL task ready — carousel affiche");
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
        kernel_tick();
        if (now - last_rtc_check >= 1000) {
            last_rtc_check = now;
            ui_status_bar_tick();
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// ── task_voice_io — Core 0, prio 2 ───────────────────────────
static void task_voice_io(void*) {
    Serial.println("[VOICE] task_voice_io started (Core 0)");
    vTaskSuspend(nullptr);
}

// ── task_ble — Core 0, prio 4 ────────────────────────────────
static void task_ble(void*) {
    ble::ble_init(
        [](const String& txt) {
            VoiceIntent vi {};
            vi.target_app = AppId::NONE;
            strncpy(vi.intent, "free_speech", sizeof(vi.intent) - 1);
            strncpy(vi.param,  txt.c_str(),  sizeof(vi.param)  - 1);
            os::kernel_post_intent(vi);
        },
        [](const String& json) { /* TODO: sync agent Nestor */ },
        [](const String& json) { /* TODO: relay LLM reponse BLE */ }
    );
    Serial.println("[BLE] task started");
    for (;;) { vTaskDelay(pdMS_TO_TICKS(10)); }
}

// ── task_network — Core 0, prio 3 ────────────────────────────
static void task_network(void*) {
    // Delai 5 s pour laisser BLE allouer ses buffers DMA
    // avant que WiFi tente esp_wifi_init() — evite ESP_ERR_NO_MEM
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Desactive le power-save WiFi pour eviter conflits DMA avec BLE
    esp_wifi_set_ps(WIFI_PS_NONE);

    WifiMgr::setCallbacks(
        []() {
            Serial.println("[NET] WiFi connected");
            vTaskDelay(pdMS_TO_TICKS(1000));
            time_t epoch = WifiMgr::syncNtp();
            if (epoch > 1700000000UL) {
                hal::rtc_sync_from_ntp(epoch);
                os::kernel_set_time_valid(true);
                os::kernel_schedule_next_reminder();
                Scheduler::rescheduleAll();
                Serial.printf("[NET] NTP sync OK — epoch %lu\n",
                              (unsigned long)epoch);
            } else {
                Serial.println("[NET] NTP sync failed, RTC time kept");
            }
        },
        []() { Serial.println("[NET] WiFi disconnected"); }
    );
    WifiMgr::connect();
    Serial.println("[NET] task started");
    for (;;) {
        WifiMgr::tick();
        if (!WifiMgr::isConnected()) {
            // Reconnexion toutes les 30 s seulement (evite spam)
            vTaskDelay(pdMS_TO_TICKS(30000));
            WifiMgr::reconnect();
        } else {
            vTaskDelay(pdMS_TO_TICKS(30000));
        }
    }
}

// ── os_start ──────────────────────────────────────────────────
void os_start() {
    Serial.println("[OS] Starting FreeRTOS tasks...");
    xTaskCreatePinnedToCore(task_ui_lvgl,  "ui_lvgl",  12288, nullptr, 5, &_h_ui,    1);
    xTaskCreatePinnedToCore(task_os_main,  "os_main",   8192, nullptr, 3, &_h_os,    1);
    xTaskCreatePinnedToCore(task_voice_io, "voice_io",  4096, nullptr, 2, &_h_voice, 0);
    xTaskCreatePinnedToCore(task_ble,      "ble",        8192, nullptr, 4, &_h_ble,   0);
    xTaskCreatePinnedToCore(task_network,  "network",   6144, nullptr, 3, &_h_net,   0);
    Serial.println("[OS] All tasks launched");
}

} // namespace os
