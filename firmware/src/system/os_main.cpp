// ============================================================
// CompagnonV2 — system/os_main.cpp
// B2  — syncNtp() appelé dans callback WiFi connected
// R2  — apps_register_all() retiré d'ici (fait dans main.cpp)
// R3  — lv_init() conservé ici (display.cpp ne l'appelle pas)
//        + guard _lv_initialized pour éviter double init
// R5  — stack sizes augmentées : ui→12288, os→8192, ble→8192
// W2  — WifiMgr::tick() appelé dans task_network
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
// R3 — display.cpp ne fait pas lv_init() → on l'appelle ici
//      guard statique pour prévenir tout double appel futur
static void task_ui_lvgl(void*) {
    static bool _lv_initialized = false;
    if (!_lv_initialized) {
        lv_init();
        _lv_initialized = true;
    }
    ui::status_bar_init();
    ui::launcher_init();
    ui::notification_init();
    Serial.println("[UI] LVGL ready");
    for (;;) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// ── task_os_main — Core 1, prio 3 ────────────────────────────
// R2 — apps_register_all() retiré : déjà appelé dans main.cpp setup()
static void task_os_main(void*) {
    kernel_init();  // inclut voice::init() avec STT callback
    uint32_t last_rtc_check = 0;
    for (;;) {
        uint32_t now = millis();
        kernel_tick();
        if (now - last_rtc_check >= 1000) {
            last_rtc_check = now;
            ui::status_bar_tick();
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// ── task_voice_io — Core 0, prio 2 ───────────────────────────
static void task_voice_io(void*) {
    Serial.println("[VOICE] task_voice_io started (Core 0)");
    // voice::init() est fait dans kernel_init() (task_os_main, Core 1).
    // La boucle interne est gérée par la tâche FreeRTOS interne au voice_engine.
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
// B2 — WifiMgr::syncNtp() appelé dans le callback connected
// W2 — WifiMgr::tick() dans la boucle pour maintenir _connected à jour
static void task_network(void*) {
    WifiMgr::setCallbacks(
        []() {
            // B2 — NTP via syncNtp() (retourne epoch UTC réel)
            Serial.println("[NET] WiFi connected");
            vTaskDelay(pdMS_TO_TICKS(1000)); // attendre stabilisation DHCP
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
        WifiMgr::tick(); // W2 — maintient l'état _connected + fire callbacks
        if (!WifiMgr::isConnected()) WifiMgr::reconnect();
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

// ── os_start ──────────────────────────────────────────────────
void os_start() {
    Serial.println("[OS] Starting FreeRTOS tasks...");
    // R5 — stacks augmentées : ui 12288, os 8192, ble 8192
    xTaskCreatePinnedToCore(task_ui_lvgl,  "ui_lvgl",  12288, nullptr, 5, &_h_ui,    1);
    xTaskCreatePinnedToCore(task_os_main,  "os_main",   8192, nullptr, 3, &_h_os,    1);
    xTaskCreatePinnedToCore(task_voice_io, "voice_io",  4096, nullptr, 2, &_h_voice, 0);
    xTaskCreatePinnedToCore(task_ble,      "ble",        8192, nullptr, 4, &_h_ble,   0);
    xTaskCreatePinnedToCore(task_network,  "network",   6144, nullptr, 3, &_h_net,   0);
    Serial.println("[OS] All tasks launched");
}

} // namespace os
