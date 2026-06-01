// ============================================================
// CompagnonV2 — system/os_main.cpp
// Corrections :
//   1. lv_indev_create pour le touch CST9220 → LVGL reçoit
//      les événements tactiles et les dispatch aux widgets
//   2. WiFi OOM : délai 10s + esp_bt_controller_mem_release
//      pour libérer ~60KB de RAM BT Classic avant init WiFi
//   3. app_launch thread-safe : passage par intent_queue
//      (LVGL + kernel tournent tous deux sur Core 1 mais
//      app_launch venant d'un btn_tick est déjà sur Core 1
//      — on ajoute lock LVGL pour lv_scr_load_anim)
// ============================================================
#include "os_main.h"
#include "os_kernel.h"
#include "../hal/rtc.h"
#include "../hal/touch.h"
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
#include <esp_bt.h>
#include <Arduino.h>

namespace os {

static TaskHandle_t _h_ui    = nullptr;
static TaskHandle_t _h_os    = nullptr;
static TaskHandle_t _h_voice = nullptr;
static TaskHandle_t _h_ble   = nullptr;
static TaskHandle_t _h_net   = nullptr;

// ── LVGL input device : touch CST9220 ──────────────────────
static void _touch_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    uint16_t x = 0, y = 0;
    if (hal::touch_read(x, y)) {
        data->point.x = (int32_t)x;
        data->point.y = (int32_t)y;
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void task_ui_lvgl(void*) {
    // Enregistrement du périphérique d'entrée touch
    lv_indev_t* touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, _touch_read_cb);

    ui_status_bar_init();
    ui_launcher_init();
    Serial.println("[UI] LVGL task ready");
    for (;;) {
        lv_timer_handler();
        ui_launcher_btn_tick();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

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
    // Attente 10s : laisser BLE s'initialiser complètement
    // avant de demander la RAM WiFi (évite ESP_ERR_NO_MEM)
    vTaskDelay(pdMS_TO_TICKS(10000));

    // Libère la RAM BT Classic (~60KB) non utilisée par NimBLE
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

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
        vTaskDelay(pdMS_TO_TICKS(30000));
        if (!WifiMgr::isConnected())
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
