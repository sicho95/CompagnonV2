// ============================================================
// CompagnonV2 — system/os_main.cpp
// fix — lv_init() supprimé (appelé une fois dans setup())
// fix — ui_status_bar_init/ui_launcher_init appelés UNE seule fois
//        dans task_ui_lvgl (ne pas les rappeler depuis le .ino)
// R5  — stack sizes : ui→12288, os→8192, ble→8192
// W2  — WifiMgr::tick() dans task_network
// B2  — syncNtp() dans callback WiFi connected
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
#include <Arduino.h>

namespace os {

static TaskHandle_t _h_ui    = nullptr;
static TaskHandle_t _h_os    = nullptr;
static TaskHandle_t _h_voice = nullptr;
static TaskHandle_t _h_ble   = nullptr;
static TaskHandle_t _h_net   = nullptr;

// ── task_ui_lvgl — Core 1, prio 5 ────────────────────────────
// C'est ici et uniquement ici que status_bar et launcher sont initialisés.
// Le .ino ne doit PAS les appeler — évite la double init.
static void task_ui_lvgl(void*) {
    ui_status_bar_init();
    ui_launcher_init();
    Serial.println("[UI] LVGL task ready — carousel affiché");
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
    // voice::init() est fait dans kernel_init() (task_os_main).
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
        [](const String& json) { /* TODO: relay LLM réponse BLE */ }
    );
    Serial.println("[BLE] task started");
    for (;;) { vTaskDelay(pdMS_TO_TICKS(10)); }
}

// ── task_network — Core 0, prio 3 ────────────────────────────
static void task_network(void*) {
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
        if (!WifiMgr::isConnected()) WifiMgr::reconnect();
        vTaskDelay(pdMS_TO_TICKS(30000));
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
