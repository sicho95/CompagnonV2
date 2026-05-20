// ============================================================
// CompagnonV2 — system/wifi_mgr.cpp
// fix: WiFi init dans une tache FreeRTOS (post-scheduler)
//
// WiFi.mode() / WiFi.begin() utilisent des semaphores internes
// crees par la tache wifi esp-idf. Si appeles depuis setup()
// avant que FreeRTOS ait schedule cette tache, on obtient :
//   assert xQueueSemaphoreTake (pxQueue==NULL) → crash/reboot
//
// Solution : on lance une tache one-shot qui attend 500ms
// (le temps que le scheduler demarre toutes les taches systeme)
// puis execute la connexion WiFi normalement.
// ============================================================
#include "wifi_mgr.h"
#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define WIFI_NAMESPACE      "wifi_mgr"
#define MAX_NETWORKS        5
#define CONNECT_TIMEOUT_MS  10000
#define WIFI_INIT_DELAY_MS  500   // laisse FreeRTOS scheduler la tache wifi esp-idf

static Preferences   _prefs;
static bool          _initialized   = false;
static int           _network_count = 0;
static int           _current_idx   = -1;
static unsigned long _connect_start = 0;
static bool          _wifi_ready    = false;  // true une fois la tache init terminee

struct SavedNetwork { String ssid; String pwd; };
static SavedNetwork _networks[MAX_NETWORKS];

static void _load_networks() {
    _prefs.begin(WIFI_NAMESPACE, true);
    _network_count = _prefs.getInt("count", 0);
    if (_network_count > MAX_NETWORKS) _network_count = MAX_NETWORKS;
    for (int i = 0; i < _network_count; i++) {
        char ks[12], kp[12];
        snprintf(ks, sizeof(ks), "ssid%d", i);
        snprintf(kp, sizeof(kp), "pwd%d",  i);
        _networks[i].ssid = _prefs.getString(ks, "");
        _networks[i].pwd  = _prefs.getString(kp, "");
    }
    _prefs.end();
}

static void _save_networks() {
    _prefs.begin(WIFI_NAMESPACE, false);
    _prefs.putInt("count", _network_count);
    for (int i = 0; i < _network_count; i++) {
        char ks[12], kp[12];
        snprintf(ks, sizeof(ks), "ssid%d", i);
        snprintf(kp, sizeof(kp), "pwd%d",  i);
        _prefs.putString(ks, _networks[i].ssid);
        _prefs.putString(kp, _networks[i].pwd);
    }
    _prefs.end();
}

static void _begin_connect(int idx) {
    if (idx < 0 || idx >= _network_count) { _current_idx = -1; return; }
    if (_networks[idx].ssid.isEmpty())    { _current_idx = -1; return; }
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false);
    WiFi.begin(_networks[idx].ssid.c_str(), _networks[idx].pwd.c_str());
    _current_idx   = idx;
    _connect_start = millis();
    Serial.printf("[WIFI] Connecting to %s...\n", _networks[idx].ssid.c_str());
}

// Tache one-shot : attend que FreeRTOS soit stable, puis lance le WiFi
static void _wifi_init_task(void* pvParam) {
    vTaskDelay(pdMS_TO_TICKS(WIFI_INIT_DELAY_MS));  // laisse le scheduler demarrer
    Serial.println("[WIFI] init task started");
    _wifi_ready = true;
    if (_network_count > 0) _begin_connect(0);
    vTaskDelete(nullptr);  // tache one-shot : se supprime elle-meme
}

void wifi_mgr_init() {
    _load_networks();
    _initialized = true;
    // Lance la tache d'init : NE PAS appeler WiFi ici (trop tot dans setup)
    xTaskCreatePinnedToCore(
        _wifi_init_task, "wifi_init", 4096, nullptr, 1, nullptr, 1
    );
    Serial.println("[WIFI] init scheduled (task +500ms)");
}

void wifi_mgr_tick() {
    if (!_initialized || !_wifi_ready) return;

    if (WiFi.status() == WL_CONNECTED) {
        if (_current_idx >= 0) {
            Serial.printf("[WIFI] Connected: %s  IP=%s\n",
                _networks[_current_idx].ssid.c_str(),
                WiFi.localIP().toString().c_str());
            _current_idx = -1;
        }
        return;
    }

    // Tentative en cours : verifier timeout
    if (_current_idx >= 0) {
        if (millis() - _connect_start < CONNECT_TIMEOUT_MS) return;
        Serial.printf("[WIFI] Timeout on %s\n", _networks[_current_idx].ssid.c_str());
        int next = _current_idx + 1;
        if (next < _network_count) _begin_connect(next);
        else {
            Serial.println("[WIFI] No network available");
            _current_idx = -1;
        }
        return;
    }

    // Retry toutes les 30s si deconnecte
    static unsigned long _last_retry = 0;
    if (_network_count > 0 && millis() - _last_retry > 30000) {
        _last_retry = millis();
        _begin_connect(0);
    }
}

void wifi_mgr_provision(const char* ssid, const char* pwd) {
    if (!ssid || !ssid[0]) return;
    for (int i = 0; i < _network_count; i++) {
        if (_networks[i].ssid == ssid) {
            _networks[i].pwd = pwd ? pwd : "";
            _save_networks();
            if (_wifi_ready) _begin_connect(i);
            return;
        }
    }
    int slot = _network_count < MAX_NETWORKS ? _network_count++ : MAX_NETWORKS - 1;
    _networks[slot].ssid = ssid;
    _networks[slot].pwd  = pwd ? pwd : "";
    _save_networks();
    Serial.printf("[WIFI] Provisioned: %s\n", ssid);
    if (_wifi_ready) _begin_connect(slot);
}

void wifi_mgr_remove_network(const char* ssid) {
    if (!ssid || !ssid[0]) return;
    for (int i = 0; i < _network_count; i++) {
        if (_networks[i].ssid == ssid) {
            for (int j = i; j < _network_count - 1; j++)
                _networks[j] = _networks[j + 1];
            _network_count--;
            _save_networks();
            Serial.printf("[WIFI] Removed: %s\n", ssid);
            return;
        }
    }
}

String wifi_mgr_list_networks() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < _network_count; i++) {
        JsonObject o = arr.add<JsonObject>();
        o["ssid"]      = _networks[i].ssid;
        o["connected"] = (WiFi.status() == WL_CONNECTED &&
                          WiFi.SSID() == _networks[i].ssid);
    }
    String out;
    serializeJson(doc, out);
    return out;
}

bool wifi_mgr_is_connected() {
    return WiFi.status() == WL_CONNECTED;
}
