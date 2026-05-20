// ============================================================
// CompagnonV2 — system/wifi_mgr.cpp
// fix DEFINITIF : WiFi appele exclusivement depuis loop()
//
// WiFi.mode()/WiFi.begin() ne sont PAS thread-safe depuis une
// tache FreeRTOS custom. Le seul contexte sur sur ESP32-Arduino
// est loop() (Core 1, contexte Arduino natif).
//
// wifi_mgr_init() : ne touche PAS au WiFi, pose juste un flag.
// wifi_mgr_tick() : appele depuis loop(), execute WiFi.begin()
//   la premiere fois apres WIFI_BOOT_DELAY_MS de boot pour laisser
//   le reste de setup() se terminer proprement.
// ============================================================
#include "wifi_mgr.h"
#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoJson.h>

#define WIFI_NAMESPACE      "wifi_mgr"
#define MAX_NETWORKS        5
#define CONNECT_TIMEOUT_MS  10000
#define WIFI_BOOT_DELAY_MS  1500   // attend que loop() tourne depuis 1.5s

static bool          _initialized    = false;
static bool          _wifi_started   = false;  // premier WiFi.begin() fait
static int           _network_count  = 0;
static int           _current_idx    = -1;
static unsigned long _connect_start  = 0;

struct SavedNetwork { String ssid; String pwd; };
static SavedNetwork _networks[MAX_NETWORKS];

static void _load_networks() {
    Preferences prefs;
    prefs.begin(WIFI_NAMESPACE, true);
    _network_count = prefs.getInt("count", 0);
    if (_network_count > MAX_NETWORKS) _network_count = MAX_NETWORKS;
    for (int i = 0; i < _network_count; i++) {
        char ks[12], kp[12];
        snprintf(ks, sizeof(ks), "ssid%d", i);
        snprintf(kp, sizeof(kp), "pwd%d",  i);
        _networks[i].ssid = prefs.getString(ks, "");
        _networks[i].pwd  = prefs.getString(kp, "");
    }
    prefs.end();
}

static void _save_networks() {
    Preferences prefs;
    prefs.begin(WIFI_NAMESPACE, false);
    prefs.putInt("count", _network_count);
    for (int i = 0; i < _network_count; i++) {
        char ks[12], kp[12];
        snprintf(ks, sizeof(ks), "ssid%d", i);
        snprintf(kp, sizeof(kp), "pwd%d",  i);
        prefs.putString(ks, _networks[i].ssid);
        prefs.putString(kp, _networks[i].pwd);
    }
    prefs.end();
}

// Appele UNIQUEMENT depuis wifi_mgr_tick() (contexte loop())
static void _begin_connect(int idx) {
    if (idx < 0 || idx >= _network_count) { _current_idx = -1; return; }
    if (_networks[idx].ssid.isEmpty())    { _current_idx = -1; return; }
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false);
    WiFi.begin(_networks[idx].ssid.c_str(), _networks[idx].pwd.c_str());
    _current_idx  = idx;
    _connect_start = millis();
    Serial.printf("[WIFI] Connecting to %s...\n", _networks[idx].ssid.c_str());
}

void wifi_mgr_init() {
    // NE PAS appeler WiFi ici — on est encore dans setup()
    // wifi_mgr_tick() s'en chargera depuis loop()
    _load_networks();
    _initialized = true;
    Serial.println("[WIFI] init OK (connexion differee dans loop)");
}

void wifi_mgr_tick() {
    if (!_initialized) return;

    // Attendre WIFI_BOOT_DELAY_MS depuis le debut du boot
    // pour etre certain que setup() est termine et que loop() tourne
    if (!_wifi_started) {
        if (millis() < WIFI_BOOT_DELAY_MS) return;
        _wifi_started = true;
        if (_network_count > 0) _begin_connect(0);
        else Serial.println("[WIFI] Aucun reseau configure");
        return;
    }

    // Connexion reussie
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
            // Reconnexion immediate seulement si on est dans loop()
            if (_wifi_started) _begin_connect(i);
            return;
        }
    }
    int slot = _network_count < MAX_NETWORKS ? _network_count++ : MAX_NETWORKS - 1;
    _networks[slot].ssid = ssid;
    _networks[slot].pwd  = pwd ? pwd : "";
    _save_networks();
    Serial.printf("[WIFI] Provisioned: %s\n", ssid);
    if (_wifi_started) _begin_connect(slot);
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
