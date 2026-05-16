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

// ── Core 1 ───────────────────────────────────────────────
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

// ── Core 0 ───────────────────────────────────────────────
static void task_voice_io(void*) {
    voice::voice_engine_init();
    Serial.println("[VOICE] task started on Core 0");
    voice::voice_engine_run();
    vTaskDelete(nullptr);
}

// ── BLE : callbacks vers le kernel ───────────────────────────
// on_text  : texte saisi depuis la PWA → intent vocal
// on_agent : sync config/réglages depuis la PWA (JSON)
//            • {"type":"wifi", "ssid":"...", "pass":"..."}
//            • {"type":"groq", "key":"..."}
// on_llm   : relay LLM (réponse stream depuis PWA)
static void _ble_on_text(const String& text) {
    VoiceIntent intent;
    intent.target_app = AppId::NESTOR;
    strncpy(intent.intent, "text_input", sizeof(intent.intent));
    strncpy(intent.param,  text.c_str(), sizeof(intent.param));
    kernel_post_intent(intent);
}

static void _ble_on_agent(const String& json) {
    // Parser le JSON et écrire dans NVS — c'est le canal prod pour
    // pousser les credentials sans recompiler
    // Format attendu de la PWA :
    //   {"type":"wifi",  "ssid":"...", "pass":"..."}
    //   {"type":"groq",  "key":"..."}
    //   {"type":"reminder", "label":"...", "datetime":1234567890,
    //                       "advance":5, "enabled":true}
    Serial.printf("[BLE] agent_sync: %s\n", json.c_str());
    // TODO — à implanter avec ArduinoJson dans une prochaine étape
    // pour l'instant on logue uniquement
}

static void _ble_on_llm(const String& json) {
    Serial.printf("[BLE] llm_relay: %s\n", json.c_str());
}

static void task_ble(void*) {
    ble::ble_init(_ble_on_text, _ble_on_agent, _ble_on_llm);
    Serial.println("[BLE] task started");
    // ble_manager tourne en interne (callbacks BLE stack)
    // Cette tâche reste vivante pour les notify éventuels
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ── Network ───────────────────────────────────────────────
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

// ── os_start ───────────────────────────────────────────────
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
