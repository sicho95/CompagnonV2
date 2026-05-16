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
#include "../net/ble_manager.h"
#include "../storage/nvs_store.h"
#include "../storage/reminder_store.h"
#include "../system/scheduler.h"
#include <lvgl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ArduinoJson.h>
#include <Arduino.h>

namespace os {

static TaskHandle_t _h_ui    = nullptr;
static TaskHandle_t _h_os    = nullptr;
static TaskHandle_t _h_voice = nullptr;
static TaskHandle_t _h_ble   = nullptr;
static TaskHandle_t _h_net   = nullptr;

// ── Core 1 ───────────────────────────────────────────────────────────────────
static void task_ui_lvgl(void*) {
    lv_init();
    ui::status_bar_init();
    ui::launcher_init();
    Serial.println("[UI] LVGL ready");
    for (;;) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

static void task_os_main(void*) {
    kernel_init();
    uint32_t last_rtc_check = 0;
    for (;;) {
        kernel_tick();
        uint32_t now = millis();
        if (now - last_rtc_check >= 1000) {
            last_rtc_check = now;
            ui::status_bar_tick();
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// ── Core 0 ───────────────────────────────────────────────────────────────────
static void task_voice_io(void*) {
    voice::voice_engine_init();
    Serial.println("[VOICE] task started on Core 0");
    voice::voice_engine_run();
    vTaskDelete(nullptr);
}

// ── BLE callbacks ─────────────────────────────────────────────────────────────
// on_text : texte saisi depuis la PWA → intent vocal vers Nestor
static void _ble_on_text(const String& text) {
    VoiceIntent intent;
    intent.target_app = AppId::NESTOR;
    strncpy(intent.intent, "text_input", sizeof(intent.intent));
    strncpy(intent.param,  text.c_str(), sizeof(intent.param));
    kernel_post_intent(intent);
}

// on_agent : canal de configuration/provisioning depuis la PWA
// Formats JSON supportés :
//   {"type":"wifi",     "ssid":"...",    "pass":"..."}
//   {"type":"groq",     "key":"..."}
//   {"type":"bourse",   "key":"..."}
//   {"type":"meteo",    "key":"..."}
//   {"type":"tuya",     "access_id":"...", "access_key":"...", "region":"eu"}
//   {"type":"ecovacs",  "account":"...",   "password":"...",   "continent":"eu"}
//   {"type":"reminder", "label":"...",    "datetime":1234567890,
//                        "advance":5,      "enabled":true}
static void _ble_on_agent(const String& json) {
    Serial.printf("[BLE] agent_sync: %s\n", json.c_str());

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("[BLE] JSON parse error: %s\n", err.c_str());
        return;
    }

    const char* type = doc["type"] | "";

    // ── Credentials WiFi ─────────────────────────────────────────────────────
    if (strcmp(type, "wifi") == 0) {
        const char* ssid = doc["ssid"] | "";
        const char* pass = doc["pass"] | "";
        if (strlen(ssid) > 0) {
            NvsStore::setString("wifi", "ssid", ssid);
            NvsStore::setString("wifi", "pass", pass);
            Serial.println("[BLE] WiFi credentials updated in NVS");
            // Reconnexion immédiate
            net::wifi_reconnect();
        }
    }
    // ── Clé Groq ─────────────────────────────────────────────────────────────
    else if (strcmp(type, "groq") == 0) {
        const char* key = doc["key"] | "";
        if (strlen(key) > 0) {
            NvsStore::setString("app", "groq_api_key", key);
            Serial.println("[BLE] Groq API key updated in NVS");
        }
    }
    // ── Clé Twelve Data ──────────────────────────────────────────────────────
    else if (strcmp(type, "bourse") == 0) {
        const char* key = doc["key"] | "";
        if (strlen(key) > 0) {
            NvsStore::setString("bourse", "td_api_key", key);
            Serial.println("[BLE] Twelve Data key updated in NVS");
        }
    }
    // ── Clé WeatherAPI ───────────────────────────────────────────────────────
    else if (strcmp(type, "meteo") == 0) {
        const char* key = doc["key"] | "";
        if (strlen(key) > 0) {
            NvsStore::setString("meteo", "weather_api_key", key);
            Serial.println("[BLE] WeatherAPI key updated in NVS");
        }
    }
    // ── Credentials Tuya ─────────────────────────────────────────────────────
    else if (strcmp(type, "tuya") == 0) {
        const char* id  = doc["access_id"]  | "";
        const char* key = doc["access_key"] | "";
        const char* reg = doc["region"]     | "eu";
        if (strlen(id) > 0) {
            NvsStore::setString("tuya", "access_id",  id);
            NvsStore::setString("tuya", "access_key", key);
            NvsStore::setString("tuya", "region",     reg);
            Serial.println("[BLE] Tuya credentials updated in NVS");
        }
    }
    // ── Credentials Ecovacs ──────────────────────────────────────────────────
    else if (strcmp(type, "ecovacs") == 0) {
        const char* acc  = doc["account"]   | "";
        const char* pass = doc["password"]  | "";
        const char* cont = doc["continent"] | "eu";
        if (strlen(acc) > 0) {
            NvsStore::setString("ecovacs", "account",   acc);
            NvsStore::setString("ecovacs", "password",  pass);
            NvsStore::setString("ecovacs", "continent", cont);
            Serial.println("[BLE] Ecovacs credentials updated in NVS");
        }
    }
    // ── Reminder créé depuis la PWA ───────────────────────────────────────────
    else if (strcmp(type, "reminder") == 0) {
        Reminder r;
        r.label           = doc["label"]   | "Rappel";
        r.datetime        = (time_t)(long)doc["datetime"];
        r.advance_minutes = doc["advance"] | 0;
        r.enabled         = doc["enabled"] | true;
        if (r.datetime > 0 && ReminderStore::add(r)) {
            Scheduler::scheduleReminder(r);
            Serial.printf("[BLE] Reminder added: %s (epoch %ld)\n",
                          r.label.c_str(), (long)r.datetime);
        } else {
            Serial.println("[BLE] Reminder add failed");
        }
    }
    else {
        Serial.printf("[BLE] Unknown agent type: %s\n", type);
    }
}

static void _ble_on_llm(const String& json) {
    Serial.printf("[BLE] llm_relay: %s\n", json.c_str());
}

static void task_ble(void*) {
    ble::ble_init(_ble_on_text, _ble_on_agent, _ble_on_llm);
    Serial.println("[BLE] task started");
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ── Network ───────────────────────────────────────────────────────────────────
static void task_network(void*) {
    net::wifi_init();
    Serial.println("[NET] task started");
    bool ntp_done = false;
    for (;;) {
        net::wifi_tick();
        if (!ntp_done && net::wifi_is_connected()) {
            time_t epoch = net::wifi_get_ntp_epoch();
            if (epoch > 0) {
                hal::rtc_sync_from_ntp(epoch);
                os::kernel_set_time_valid(true);
                os::kernel_schedule_next_reminder();
                ntp_done = true;
                Serial.println("[NET] NTP sync done");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ── os_start ──────────────────────────────────────────────────────────────────
void os_start() {
    Serial.println("[OS] Starting FreeRTOS tasks...");
    xTaskCreatePinnedToCore(task_ui_lvgl,  "ui_lvgl",  8192, nullptr, 5, &_h_ui,    1);
    xTaskCreatePinnedToCore(task_os_main,  "os_main",  4096, nullptr, 3, &_h_os,    1);
    xTaskCreatePinnedToCore(task_voice_io, "voice_io", 8192, nullptr, 6, &_h_voice, 0);
    xTaskCreatePinnedToCore(task_ble,      "ble",      6144, nullptr, 4, &_h_ble,   0);
    xTaskCreatePinnedToCore(task_network,  "network",  6144, nullptr, 3, &_h_net,   0);
    Serial.println("[OS] All tasks launched");
}

} // namespace os
