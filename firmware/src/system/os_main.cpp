// ============================================================
// CompagnonV2 — system/os_main.cpp
// Fix LVGL9 : lv_indev_set_display() obligatoire pour que le
// touch soit rattaché au display et génère des événements.
// Fix thread-safety LVGL : dispatch_flush() dans task_ui_lvgl
// Fix WiFi : stop reconnect quand pas de SSID
// ============================================================
#include "os_main.h"
#include "os_kernel.h"
#include "../hal/rtc.h"
#include "../hal/touch.h"
#include "../ui/status_bar.h"
#include "../ui/launcher.h"
#include "../ui/notification_mgr.h"
#include "../ui/ui_dispatch.h"
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

// —— Touch read callback ——
static void _touch_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    (void)indev;
    uint16_t x = 0, y = 0;
    if (hal::touch_read(x, y)) {
        data->point.x = (int32_t)x;
        data->point.y = (int32_t)y;
        data->state   = LV_INDEV_STATE_PRESSED;
        ui_launcher_touch(true, x, y);
        static uint32_t _last_log = 0;
        if (millis() - _last_log > 200) {
            Serial.printf("[TOUCH] x=%d y=%d\n", x, y);
            _last_log = millis();
        }
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        ui_launcher_touch(false, 0, 0);
    }
}

// —— Tâche LVGL (Core 1) — SEULE à appeler des fonctions LVGL ——
static void task_ui_lvgl(void*) {
    ui::dispatch_init();

    // LVGL9 : l'indev DOIT être rattaché au display via lv_indev_set_display()
    // Sans ça, le touch est créé mais LVGL ne lui envoie aucun événement.
    lv_indev_t* touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, _touch_read_cb);
    lv_indev_set_display(touch_indev, lv_display_get_default());
    Serial.println("[TOUCH] LVGL probe armed");

    ui_status_bar_init();
    ui_launcher_init();
    Serial.println("[UI] LVGL task ready");
    uint32_t last_status_tick = 0;
    for (;;) {
        ui::dispatch_flush();   // exécute les lv_scr_load postés par os_kernel
        lv_timer_handler();
        ui::notification_tick();
        uint32_t now = millis();
        if (now - last_status_tick >= 1000) {
            last_status_tick = now;
            ui_status_bar_tick();
        }
        ui_launcher_btn_tick();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

static void task_os_main(void*) {
    kernel_init();
    for (;;) {
        kernel_tick();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static void task_voice_io(void*) {
    Serial.println("[VOICE] task_voice_io started (Core 0)");
    vTaskSuspend(nullptr);
}

static void task_ble(void*) {
    ble::ble_init(
        [](const String& txt) {
            VoiceIntent vi {};
            vi.target_app = AppId::NONE;
            strncpy(vi.intent, "free_speech", sizeof(vi.intent) - 1);
            strncpy(vi.param,  txt.c_str(),  sizeof(vi.param)  - 1);
            os::kernel_post_intent(vi);
        },
        [](const String& json) { },
        [](const String& json) { }
    );
    Serial.println("[BLE] task started");
    for (;;) { vTaskDelay(pdMS_TO_TICKS(10)); }
}

static void task_network(void*) {
    vTaskDelay(pdMS_TO_TICKS(10000));
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
    bool ok = WifiMgr::connect();
    Serial.println("[NET] task started");
    for (;;) {
        WifiMgr::tick();
        vTaskDelay(pdMS_TO_TICKS(30000));
        // Ne tenter reconnect que si WiFi a déjà réussi à s'initialiser
        if (ok && !WifiMgr::isConnected())
            WifiMgr::reconnect();
    }
}

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
