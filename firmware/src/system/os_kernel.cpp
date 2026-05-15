// ============================================================
// CompagnonV2 — system/os_kernel.cpp
// ============================================================
#include "os_kernel.h"
#include "../hal/rtc.h"
#include "../hal/pmu.h"
#include "../../include/pins.h"
#include "../apps/app_nestor.h"
#include "../apps/app_radars.h"
#include "../apps/app_bourse.h"
#include "../apps/app_meteo.h"
#include "../apps/app_rappels.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <Arduino.h>

namespace os {

// ── Registre apps ────────────────────────────────────────────
static AppDesc _apps[(int)AppId::COUNT];
static AppId   _current_app = AppId::NONE;
static bool    _silent      = false;
static bool    _time_valid  = false;

// ── Queue intents vocaux (depuis task_voice_io Core 0) ───────
static QueueHandle_t _intent_queue = nullptr;

void kernel_init() {
    _intent_queue = xQueueCreate(8, sizeof(VoiceIntent));
    // Charger mode silencieux depuis NVS
    nvs_handle_t nvs;
    if (nvs_open("os_cfg", NVS_READONLY, &nvs) == ESP_OK) {
        uint8_t s = 0;
        nvs_get_u8(nvs, "silent", &s);
        _silent = (s == 1);
        nvs_close(nvs);
    }
    _time_valid = hal::rtc_is_valid();
    Serial.printf("[KERNEL] init — silent=%d time_valid=%d\n", _silent, _time_valid);
}

void apps_register_all() {
    // Enregistrement statique — AUCUNE allocation LVGL ici
    _apps[(int)AppId::NESTOR]  = { AppId::NESTOR,  "Nestor",   "🤖",
        app_nestor_start,  app_nestor_stop,  app_nestor_intent };
    _apps[(int)AppId::RADARS]  = { AppId::RADARS,  "Radars",   "📡",
        app_radars_start,  app_radars_stop,  nullptr };
    _apps[(int)AppId::BOURSE]  = { AppId::BOURSE,  "Bourse",   "📈",
        app_bourse_start,  app_bourse_stop,  nullptr };
    _apps[(int)AppId::METEO]   = { AppId::METEO,   "Météo",    "🌤",
        app_meteo_start,   app_meteo_stop,   nullptr };
    _apps[(int)AppId::RAPPELS] = { AppId::RAPPELS, "Rappels",  "⏰",
        app_rappels_start, app_rappels_stop, app_rappels_intent };
    Serial.println("[KERNEL] 5 apps registered");
}

bool app_launch(AppId id) {
    if (id == AppId::NONE || (int)id >= (int)AppId::COUNT) return false;
    // Stopper l'app courante si nécessaire
    if (_current_app != AppId::NONE) {
        _apps[(int)_current_app].stop();
    }
    _current_app = id;
    bool ok = _apps[(int)id].start();
    if (!ok) { _current_app = AppId::NONE; }
    return ok;
}

void app_close_current() {
    if (_current_app == AppId::NONE) return;
    _apps[(int)_current_app].stop();
    _current_app = AppId::NONE;
    // Le launcher reprend automatiquement (task_ui_lvgl)
}

AppId app_current() { return _current_app; }

void kernel_post_intent(const VoiceIntent& intent) {
    if (_intent_queue) xQueueSend(_intent_queue, &intent, 0);
}

void kernel_set_silent(bool s) {
    _silent = s;
    nvs_handle_t nvs;
    if (nvs_open("os_cfg", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_u8(nvs, "silent", s ? 1 : 0);
        nvs_commit(nvs);
        nvs_close(nvs);
    }
}
bool kernel_is_silent()      { return _silent; }
void kernel_set_time_valid(bool v) { _time_valid = v; }
bool kernel_time_is_valid()  { return _time_valid; }

void kernel_schedule_next_reminder() {
    // Lire prochain rappel depuis app_rappels, programmer alarme RTC
    // + esp_sleep_enable_timer_wakeup si en light sleep
    extern time_t app_rappels_next_epoch();
    time_t next = app_rappels_next_epoch();
    if (next > 0) {
        hal::rtc_set_alarm(next);
        Serial.printf("[KERNEL] Next reminder scheduled at epoch %lu\n",
            (unsigned long)next);
    } else {
        hal::rtc_clear_alarm();
    }
}

void kernel_tick() {
    // Traiter les intents vocaux en attente (appelé depuis task_os_main)
    VoiceIntent intent;
    while (xQueueReceive(_intent_queue, &intent, 0) == pdTRUE) {
        Serial.printf("[KERNEL] Intent: app=%d intent=%s param=%s\n",
            (int)intent.target_app, intent.intent, intent.param);
        // Lancer l'app cible si besoin
        if (intent.target_app != AppId::NONE &&
            intent.target_app != _current_app) {
            app_launch(intent.target_app);
        }
        // Router l'intent vers l'app
        AppId target = (intent.target_app != AppId::NONE)
            ? intent.target_app : _current_app;
        if (target != AppId::NONE && _apps[(int)target].handle_intent) {
            _apps[(int)target].handle_intent(intent.intent, intent.param);
        }
    }
}

} // namespace os
