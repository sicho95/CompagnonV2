// ============================================================
// CompagnonV2 — system/wifi_mgr.cpp
// Gère la connexion multi-réseau avec persistance via Preferences
// fix: connexion non-bloquante — WiFi.begin() sans while(delay())
//      La boucle bloquante dans setup() crashait FreeRTOS avec
//      assert xQueueSemaphoreTake (pxQueue==NULL) car les sémaphores
//      internes de la tâche wifi esp-idf n'étaient pas encore créés.
// ============================================================
#include "wifi_mgr.h"
#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoJson.h>

#define WIFI_NAMESPACE      "wifi_mgr"
#define MAX_NETWORKS        5
#define CONNECT_TIMEOUT_MS  10000

static Preferences _prefs;
static bool        _initialized   = false;
static int         _network_count = 0;
static int         _current_idx   = -1;       // réseau en cours de tentative
static unsigned long _connect_start = 0;       // timestamp du WiFi.begin() actif

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

// Lance la tentative de connexion au réseau idx — NON BLOQUANT
static void _begin_connect(int idx) {
    if (idx < 0 || idx >= _network_count) { _current_idx = -1; return; }
    if (_networks[idx].ssid.isEmpty())    { _current_idx = -1; return; }
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false);
    WiFi.begin(_networks[idx].ssid.c_str(), _networks[idx].pwd.c_str());
    _current_idx   = idx;
    _connect_start = millis();
    Serial.printf("[WIFI] Connecting to %s (async)...\n", _networks[idx].ssid.c_str());
}

void wifi_mgr_init() {
    _load_networks();
    _initialized = true;
    // Lance la première tentative sans bloquer — wifi_mgr_tick() gère la suite
    if (_network_count > 0) _begin_connect(0);
}

void wifi_mgr_tick() {
    if (!_initialized) return;

    // Connexion réussie
    if (WiFi.status() == WL_CONNECTED) {
        if (_current_idx >= 0) {
            Serial.printf("[WIFI] Connected: %s  IP=%s\n",
                _networks[_current_idx].ssid.c_str(),
                WiFi.localIP().toString().c_str());
            _current_idx = -1;   // marque comme résolu
        }
        return;
    }

    // Tentative en cours : vérifier timeout
    if (_current_idx >= 0) {
        if (millis() - _connect_start < CONNECT_TIMEOUT_MS) return; // attendre
        // Timeout — essayer le réseau suivant
        Serial.printf("[WIFI] Timeout on %s\n", _networks[_current_idx].ssid.c_str());
        int next = _current_idx + 1;
        if (next < _network_count) {
            _begin_connect(next);
        } else {
            Serial.println("[WIFI] No network available");
            _current_idx = -1;
        }
        return;
    }

    // Pas de tentative en cours et pas connecté : retry toutes les 30s
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
            Serial.printf("[WIFI] Updated: %s\n", ssid);
            _begin_connect(i);
            return;
        }
    }
    int slot = _network_count < MAX_NETWORKS ? _network_count++ : MAX_NETWORKS - 1;
    _networks[slot].ssid = ssid;
    _networks[slot].pwd  = pwd ? pwd : "";
    _save_networks();
    Serial.printf("[WIFI] Provisioned: %s\n", ssid);
    _begin_connect(slot);
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
