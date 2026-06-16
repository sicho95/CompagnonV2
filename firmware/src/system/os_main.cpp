// ============================================================
// CompagnonV2 — system/os_main.cpp
// Fix LVGL9 : lv_indev_set_display() obligatoire pour que le
// touch soit rattaché au display et génère des événements.
// Fix thread-safety LVGL : dispatch_flush() dans task_ui_lvgl
// Fix WiFi : stop reconnect quand pas de SSID
//
// TOUCH COORDS — Option B (évolutive QMI8658) :
// _touch_read_cb applique la transformation DIRECTE correspondant
// à la rotation LVGL courante (lue dynamiquement via display_get_rotation()).
// LVGL applique ensuite la transformation INVERSE via lv_indev_set_display().
// Les deux s'annulent -> zones de hit correctes quelle que soit la rotation.
// Supporte la rotation dynamique (QMI8658) sans aucune modification future.
//
// Formules (W = hor_res, H = ver_res, lus dynamiquement) :
//   ROT_0   : lv_x = raw_x,       lv_y = raw_y
//   ROT_90  : lv_x = raw_y,       lv_y = (W-1) - raw_x
//   ROT_180 : lv_x = (W-1)-raw_x, lv_y = (H-1) - raw_y
//   ROT_270 : lv_x = (H-1)-raw_y, lv_y = raw_x
// ============================================================
#include "os_main.h"
#include "os_kernel.h"
#include "../hal/rtc.h"
#include "../hal/touch.h"
#include "../hal/display.h"
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

// Applique la transformation directe correspondant à la rotation LVGL courante.
// W et H sont lus dynamiquement depuis le display pour éviter tout hardcodage.
// LVGL applique la transformation inverse via lv_indev_set_display() :
// les deux s'annulent -> coords correctes quelle que soit la rotation.
static void _apply_rotation(uint16_t raw_x, uint16_t raw_y,
                            int32_t& out_x, int32_t& out_y) {
    lv_display_t* disp = hal::display_get();
    const lv_display_rotation_t rot = hal::display_get_rotation();
    const int32_t W = disp ? (int32_t)lv_display_get_horizontal_resolution(disp) : 480;
    const int32_t H = disp ? (int32_t)lv_display_get_vertical_resolution(disp)   : 480;
    const int32_t rx = (int32_t)raw_x;
    const int32_t ry = (int32_t)raw_y;
    switch (rot) {
        case LV_DISPLAY_ROTATION_0:
            out_x = rx;
            out_y = ry;
            break;
        case LV_DISPLAY_ROTATION_90:
            out_x = ry;
            out_y = (W - 1) - rx;
            break;
        case LV_DISPLAY_ROTATION_180:
            out_x = (W - 1) - rx;
            out_y = (H - 1) - ry;
            break;
        case LV_DISPLAY_ROTATION_270:
            out_x = (H - 1) - ry;
            out_y = rx;
            break;
        default:
            out_x = rx;
            out_y = ry;
            break;
    }
}

// —— Touch read callback ——
static void _touch_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    (void)indev;
    uint16_t raw_x = 0, raw_y = 0;
    if (hal::touch_read(raw_x, raw_y)) {
        int32_t x = 0, y = 0;
        _apply_rotation(raw_x, raw_y, x, y);
        data->point.x = x;
        data->point.y = y;
        data->state   = LV_INDEV_STATE_PRESSED;
        static uint32_t _last_log = 0;
        if (millis() - _last_log > 200) {
            Serial.printf("[TOUCH] x=%d y=%d (raw=%d,%d)\n", x, y, raw_x, raw_y);
            _last_log = millis();
        }
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// —— Tâche LVGL (Core 1) — SEULE à appeler des fonctions LVGL ——
static void task_ui_lvgl(void*) {
    ui::dispatch_init();

    // LVGL9 : l'indev DOIT être rattaché au display via lv_indev_set_display()
    // pour que LVGL applique la transformation inverse sur les coords touch.
    // _touch_read_cb applique la transformation directe -> annulation -> hit correct.
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
        ui::dispatch_flush();
        lv_timer_handler();
        lv_indev_read(touch_indev);
        ui_status_bar_touch_tick();
        ui_launcher_touch_tick();
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
