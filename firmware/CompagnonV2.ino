/*
 * CompagnonV2 — ESP32-S3 Waveshare AMOLED 2.16"
 * Framework : Arduino 3.3.8 / arduino-esp32
 * LVGL      : 9.x  (inclus comme <lvgl.h> raw — lv_init() OBLIGATOIRE)
 * Board     : Waveshare ESP32-S3-Touch-AMOLED-2.16 (CO5300 + CST9220 + QMI8658 + AXP2101)
 *
 * Ordre d'init critique :
 *  0. lv_init()  — DOIT être le tout premier appel LVGL
 *  1. PMU        — rails ALDO1/ALDO3, bouton power
 *  2. NVS        — namespace "compagnon"
 *  3. Display    — lv_display_create() + buffers (apres lv_init !)
 *  4. Touch      — 500 ms post-reset, CST9220
 *  5. IMU        — QMI8658 orientation
 *  6. Audio      — I2S mic + codec (init silencieux)
 *  7. UI         — status_bar (lv_layer_top) + launcher
 *  8. Net        — wifi_mgr + BLE + OTA
 *  9. Voice      — wake word listener (Core 0)
 * 10. Reminder   — scheduler
 * 11. Orchestrateur
 * 12. Callback power → menu UI
 */

#include <lvgl.h>

#include "src/hal/display.h"
#include "src/hal/touch.h"
#include "src/hal/imu.h"
#include "src/hal/pmu.h"
#include "src/hal/audio.h"
#include "src/system/orchestrator.h"
#include "src/system/wifi_mgr.h"
#include "src/system/power_mgr.h"
#include "src/net/ota.h"
#include "src/net/ble_mgr.h"
#include "src/config/nvs_config.h"
#include "src/ui/status_bar.h"
#include "src/ui/launcher.h"
#include "src/voice/voice_engine.h"
#include "src/apps/reminders/reminders_app.h"
#include "src/apps/smarthome/smarthome_app.h"
#include "src/apps/ecovacs/ecovacs_app.h"
#include <ArduinoJson.h>
#include <WiFi.h>

// ─── Mapping noms longs PWA → clés NVS courtes (≤ 15 chars) ────────────────
struct KeyMapping { const char *pwa_name; const char *nvs_name; };
static const KeyMapping KEY_MAP[] = {
    { "GROQ_API_KEY",           NVS_KEY_GROQ        },
    { "GEMINI_API_KEY",         NVS_KEY_GEMINI      },
    { "SERPER_API_KEY",         NVS_KEY_SERPER      },
    { "OPENROUTER_API_KEY",     NVS_KEY_OPENROUTER  },
    { "TWELVE_DATA_API_KEY",    NVS_KEY_TWELVEDATA  },
    { "TWELVEDATA_API_KEY",     NVS_KEY_TWELVEDATA  },
    { "METEO_CONCEPT_API_KEY",  NVS_KEY_METEO       },
    { "METEO_API_KEY",          NVS_KEY_METEO       },
    { "SPOTIFY_CLIENT_ID",      NVS_KEY_SPOTIFY_ID  },
    { "SPOTIFY_CLIENT_SECRET",  NVS_KEY_SPOTIFY_SEC },
    { "TUYA_CLIENT_ID",         NVS_KEY_TUYA_ID     },
    { "TUYA_CLIENT_SECRET",     NVS_KEY_TUYA_SEC    },
    { "ECOVACS_EMAIL",          NVS_KEY_ECOVACS_U   },
    { "ECOVACS_PASSWORD",       NVS_KEY_ECOVACS_P   },
    { NVS_KEY_GROQ,             NVS_KEY_GROQ        },
    { NVS_KEY_GEMINI,           NVS_KEY_GEMINI      },
    { NVS_KEY_SERPER,           NVS_KEY_SERPER      },
    { NVS_KEY_OPENROUTER,       NVS_KEY_OPENROUTER  },
    { NVS_KEY_TWELVEDATA,       NVS_KEY_TWELVEDATA  },
    { NVS_KEY_METEO,            NVS_KEY_METEO       },
    { NVS_KEY_SPOTIFY_ID,       NVS_KEY_SPOTIFY_ID  },
    { NVS_KEY_SPOTIFY_SEC,      NVS_KEY_SPOTIFY_SEC },
    { NVS_KEY_TUYA_ID,          NVS_KEY_TUYA_ID     },
    { NVS_KEY_TUYA_SEC,         NVS_KEY_TUYA_SEC    },
    { NVS_KEY_ECOVACS_U,        NVS_KEY_ECOVACS_U   },
    { NVS_KEY_ECOVACS_P,        NVS_KEY_ECOVACS_P   },
    { nullptr, nullptr }
};

static const char *resolve_nvs_key(const char *pwa_key) {
    if (!pwa_key || !pwa_key[0]) return nullptr;
    for (int i = 0; KEY_MAP[i].pwa_name != nullptr; i++)
        if (strcasecmp(KEY_MAP[i].pwa_name, pwa_key) == 0)
            return KEY_MAP[i].nvs_name;
    return nullptr;
}

// ─── Pont BLE → WiFi provisioning ──────────────────────────────────────
static void ble_wifi_prov_cb(const char *json) {
    if (!json) return;
    JsonDocument doc;
    if (deserializeJson(doc, json)) return;
    const char *ssid = doc["ssid"] | doc["s"] | (const char*)nullptr;
    const char *pwd  = doc["password"] | doc["pwd"] | doc["p"] | "";
    if (!ssid || !ssid[0]) return;
    wifi_mgr_provision(ssid, pwd);
    String nets = wifi_mgr_list_networks();
    char ack[256];
    snprintf(ack, sizeof(ack),
             "{\"cmd\":\"wifi_prov_ack\",\"ok\":true,\"networks\":%s}",
             nets.c_str());
    ble_mgr_notify_agent_sync(ack);
}

// ─── Pont BLE → Agent Sync ───────────────────────────────────────────
static void ble_agent_sync_cb(const char *json) {
    if (!json) return;
    JsonDocument doc;
    if (deserializeJson(doc, json)) return;
    const char *cmd = doc["cmd"] | "";
    if (!cmd[0]) return;

    if (strcmp(cmd, "set_api_key") == 0) {
        const char *pwa_key = doc["key"] | "";
        const char *val     = doc["val"] | doc["value"] | "";
        const char *nvs_key = resolve_nvs_key(pwa_key);
        char ack[160];
        if (!nvs_key) {
            snprintf(ack, sizeof(ack),
                     "{\"cmd\":\"set_api_key_ack\",\"key\":\"%s\",\"ok\":false,\"err\":\"unknown_key\"}",
                     pwa_key);
        } else {
            bool ok = nvs_set_api_key(nvs_key, val);
            snprintf(ack, sizeof(ack),
                     "{\"cmd\":\"set_api_key_ack\",\"key\":\"%s\",\"ok\":%s}",
                     pwa_key, ok ? "true" : "false");
        }
        ble_mgr_notify_agent_sync(ack);
        return;
    }
    if (strcmp(cmd, "get_api_keys") == 0) {
        char keys_json[512]; nvs_list_api_keys_json(keys_json, sizeof(keys_json));
        char resp[600];
        snprintf(resp, sizeof(resp), "{\"cmd\":\"api_keys_status\",\"keys\":%s}", keys_json);
        ble_mgr_notify_agent_sync(resp); return;
    }
    if (strcmp(cmd, "clear_api_key") == 0) {
        const char *pwa_key = doc["key"] | "";
        const char *nvs_key = resolve_nvs_key(pwa_key);
        if (nvs_key) nvs_clear_api_key(nvs_key);
        char ack[128];
        snprintf(ack, sizeof(ack),
                 "{\"cmd\":\"clear_api_key_ack\",\"key\":\"%s\",\"ok\":true}", pwa_key);
        ble_mgr_notify_agent_sync(ack); return;
    }
    if (strcmp(cmd, "wifi_list") == 0) {
        String nets = wifi_mgr_list_networks();
        char resp[512];
        snprintf(resp, sizeof(resp),
                 "{\"cmd\":\"wifi_list_ack\",\"networks\":%s}", nets.c_str());
        ble_mgr_notify_agent_sync(resp); return;
    }
    if (strcmp(cmd, "wifi_remove") == 0) {
        const char *ssid = doc["ssid"] | doc["s"] | "";
        if (ssid[0]) {
            wifi_mgr_remove_network(ssid);
            String nets = wifi_mgr_list_networks();
            char resp[512];
            snprintf(resp, sizeof(resp),
                     "{\"cmd\":\"wifi_remove_ack\",\"ok\":true,\"networks\":%s}",
                     nets.c_str());
            ble_mgr_notify_agent_sync(resp);
        }
        return;
    }
    if (strcmp(cmd, "set_reminders") == 0) {
        reminders_app_ble_set(json);
        ble_mgr_notify_agent_sync("{\"cmd\":\"set_reminders_ack\",\"ok\":true}");
        return;
    }
    if (strcmp(cmd, "get_reminders") == 0) {
        char resp[2048];
        reminders_app_ble_get(resp, sizeof(resp));
        ble_mgr_notify_agent_sync(resp); return;
    }
    if (strcmp(cmd, "set_silent") == 0) {
        bool silent = doc["value"] | false;
        nvs_set_bool("silent_mode", silent);
        voice_engine_set_silent(silent);
        char ack[80];
        snprintf(ack, sizeof(ack),
                 "{\"cmd\":\"set_silent_ack\",\"ok\":true,\"value\":%s}",
                 silent ? "true" : "false");
        ble_mgr_notify_agent_sync(ack); return;
    }
    if (strcmp(cmd, "battery_status") == 0 || strcmp(cmd, "get_device_status") == 0) {
        int  bat_pct  = hal_pmu_battery_pct();
        bool charging = hal_pmu_is_charging();
        bool silent   = nvs_get_bool("silent_mode", false);
        String nets   = wifi_mgr_list_networks();
        char resp[400];
        snprintf(resp, sizeof(resp),
                 "{\"cmd\":\"device_status\",\"battery\":%d,\"charging\":%s,"
                 "\"wifi\":%s,\"silent\":%s,\"wifi_networks\":%s}",
                 bat_pct >= 0 ? bat_pct : 0,
                 charging ? "true" : "false",
                 WiFi.isConnected() ? "true" : "false",
                 silent ? "true" : "false",
                 nets.c_str());
        ble_mgr_notify_agent_sync(resp); return;
    }
    Serial.printf("[BLE/AGENT] cmd inconnue: %s\n", cmd);
}

// ─── Setup ─────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[BOOT] CompagnonV2 — demarrage");

    // ── 0. LVGL init — OBLIGATOIRE en tout premier avec <lvgl.h> raw ─────────────
    // Contrairement au wrapper LVGL.h (Arduino), <lvgl.h> n'appelle pas
    // lv_init() automatiquement. Sans cet appel, lv_display_create()
    // crashe sur l'offset 0x14 (champ interne non initialisé) → LoadProhibited.
    lv_init();
    Serial.println("[BOOT] LVGL init OK");

    hal_pmu_init();
    nvs_config_init();
    hal_display_init();   // lv_display_create() + buffers — sûr après lv_init()
    hal_touch_init();
    hal_imu_init();
    hal_audio_init();

    ui_status_bar_init();
    wifi_mgr_init();
    net_ota_init();
    ble_mgr_init();
    ble_mgr_set_wifi_prov_cb(ble_wifi_prov_cb);
    ble_mgr_set_agent_sync_cb(ble_agent_sync_cb);

    voice_engine_init();
    reminders_app_init();
    orchestrator_init();
    ui_launcher_init();

    hal_pmu_set_long_press_cb(ui_power_menu_show);

    Serial.println("[BOOT] Pret.");
}

// ─── Loop (Core 1) ─────────────────────────────────────────────────────────
void loop() {
    lv_tick_inc(1);      // avance le timer interne LVGL de 1ms — OBLIGATOIRE
    lv_task_handler();   // traite les événements et renders LVGL

    hal_pmu_tick();
    wifi_mgr_tick();
    net_ota_tick();
    ble_mgr_tick();
    ui_status_bar_tick();
    hal_imu_tick();

    // Rotation auto selon orientation IMU
    if (hal_imu_changed()) {
        lv_display_t* disp = hal_display_get();
        if (disp) {
            static const lv_display_rotation_t rot_map[] = {
                LV_DISPLAY_ROTATION_270, LV_DISPLAY_ROTATION_0,
                LV_DISPLAY_ROTATION_90,  LV_DISPLAY_ROTATION_180,
            };
            lv_display_set_rotation(disp, rot_map[hal_imu_orientation()]);
        }
    }

    orchestrator_tick();
    power_mgr_tick();
    reminders_app_tick();
    smarthome_app_tick();
    ecovacs_app_tick();
    ui_launcher_btn_tick();
    delay(1);
}
