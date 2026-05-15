// ============================================================
// os_main.cpp — Orchestrateur OS CompagnonV2
//
// 2 taches sur Core 1 :
//   task_ui_lvgl  (prio 5) : lv_timer_handler() toutes les 5 ms
//   task_os_main  (prio 4) : BLE, WiFi, PMU, NTP, rappels, BLE
//
// 2 taches sur Core 0 :
//   task_voice_io      (prio 5) : VAD + wake word + capture
//   task_stt_consumer  (prio 3) : Groq STT
//
// Le mutex g_lvgl_mutex sécurise tous les accès LVGL
// ============================================================
#include "os_main.h"
#include "../net/wifi_mgr.h"
#include "../net/ble_manager.h"
#include "../hal/hal_display.h"
#include "../hal/hal_pmu.h"
#include "../voice/voice_engine.h"
#include "../apps/reminders/reminder_app.h"
#include "../ui/status_bar.h"
#include "../ui/launcher.h"
#include <ArduinoJson.h>
#include <Preferences.h>
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

namespace os {

SemaphoreHandle_t g_lvgl_mutex = nullptr;

static uint32_t _last_ntp_sync      = 0;
static uint32_t _last_bat_tick      = 0;
static uint32_t _last_reminder_tick = 0;

// ── Callbacks BLE ─────────────────────────────────────────────
static void _on_ble_text(const String& text) {
    Serial.printf("[OS] BLE text → '%s'\n", text.c_str());
    // TODO: router vers agent brain
}

static void _on_ble_agent_sync(const String& json) {
    JsonDocument d;
    if (deserializeJson(d, json)) return;
    const char* cmd = d["cmd"] | "";
    if (strcmp(cmd, "set_ble_name") == 0) {
        Preferences p; p.begin("ble_config", false);
        p.putString("device_name", d["value"] | "Compagnon"); p.end();
    }
    else if (strcmp(cmd, "set_silent") == 0) {
        voice::voice_set_silent(d["value"] | false);
    }
    else if (strcmp(cmd, "get_reminders") == 0) {
        ble::ble_notify_agent_sync(apps::reminders::reminder_to_json_all());
    }
    else if (strcmp(cmd, "set_reminders") == 0) {
        apps::reminders::reminder_from_json(json);
    }
    else if (strcmp(cmd, "reminder_event") == 0) {
        const char* act = d["action"] | "";
        if (strcmp(act, "done") == 0) apps::reminders::reminder_done(d["id"] | "");
    }
}

static void _on_ble_llm_relay(const String& json) {
    Serial.printf("[OS] BLE LLM relay len=%u\n", json.length());
    // TODO: injecter la réponse dans l'agent brain
}

// ── Callbacks voix ────────────────────────────────────────────
static void _on_wake(int word_id) {
    Serial.printf("[OS] Wake word %d\n", word_id);
    hal::display_on();
    voice::voice_trigger_stt();
}

static void _on_stt(const String& text) {
    Serial.printf("[OS] STT → '%s'\n", text.c_str());
    String t = text; t.toLowerCase();
    if (t.indexOf("rappel") >= 0 || t.indexOf("rappelle") >= 0)
        Serial.println("[OS] Route → app Rappels");
    else
        Serial.println("[OS] Route → Nestor");
    // TODO: router vers agent brain
}

// ── Status JSON ───────────────────────────────────────────────
static String _build_status_json() {
    JsonDocument d;
    d["bat_pct"]  = hal::pmu_battery_pct();
    d["charging"] = hal::pmu_is_charging();
    d["wifi"]     = (WiFi.status() == WL_CONNECTED);
    d["ble"]      = ble::ble_connected();
    d["silent"]   = voice::voice_is_silent();
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    char ts[32]; strftime(ts, sizeof(ts), "%H:%M  %d/%m/%Y", t);
    d["time_str"] = ts;
    String out; serializeJson(d, out); return out;
}

// ── Tache OS main (Core 1) ────────────────────────────────────
static void _task_os_main(void*) {
    ble::ble_init(_on_ble_text, _on_ble_agent_sync, _on_ble_llm_relay);
    voice::voice_init(_on_wake, _on_stt);
    voice::voice_start_task();
    apps::reminders::app_init();
    wifi::wifi_manager_init();

    for (;;) {
        uint32_t now_ms = millis();
        wifi::wifi_manager_tick();

        if (WiFi.status() == WL_CONNECTED &&
            now_ms - _last_ntp_sync > 30 * 60 * 1000UL) {
            configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org");
            _last_ntp_sync = now_ms;
        }

        if (now_ms - _last_bat_tick > 5000) {
            _last_bat_tick = now_ms;
            hal::pmu_tick();
            String st = _build_status_json();
            if (xSemaphoreTake(g_lvgl_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                ui::status_bar_update(st);
                xSemaphoreGive(g_lvgl_mutex);
            }
            ble::ble_notify_status(st);
        }

        if (now_ms - _last_reminder_tick > 1000) {
            _last_reminder_tick = now_ms;
            apps::reminders::app_tick();
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ── Tache LVGL (Core 1) ───────────────────────────────────────
static void _task_ui_lvgl(void*) {
    hal::display_init();
    ui::launcher_init();
    ui::status_bar_init();
    for (;;) {
        if (xSemaphoreTake(g_lvgl_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            lv_timer_handler();
            xSemaphoreGive(g_lvgl_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void os_start() {
    g_lvgl_mutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(_task_ui_lvgl, "ui_lvgl",  16384, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(_task_os_main, "os_main",  16384, NULL, 4, NULL, 1);
}

} // namespace os
