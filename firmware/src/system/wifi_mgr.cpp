// ============================================================
// CompagnonV2 — system/wifi_mgr.cpp
// fix: fichier manquant — implémentation du WiFi Manager
// Gère la connexion multi-réseau avec persistance via Preferences
// ============================================================
#include "wifi_mgr.h"
#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoJson.h>

#define WIFI_NAMESPACE   "wifi_mgr"
#define MAX_NETWORKS     5
#define CONNECT_TIMEOUT_MS 8000

static Preferences _prefs;
static bool _initialized = false;

struct SavedNetwork {
    String ssid;
    String pwd;
};
static SavedNetwork _networks[MAX_NETWORKS];
static int _network_count = 0;

static void _load_networks() {
    _prefs.begin(WIFI_NAMESPACE, true);
    _network_count = _prefs.getInt("count", 0);
    if (_network_count > MAX_NETWORKS) _network_count = MAX_NETWORKS;
    for (int i = 0; i < _network_count; i++) {
        char key_s[12], key_p[12];
        snprintf(key_s, sizeof(key_s), "ssid%d", i);
        snprintf(key_p, sizeof(key_p), "pwd%d",  i);
        _networks[i].ssid = _prefs.getString(key_s, "");
        _networks[i].pwd  = _prefs.getString(key_p, "");
    }
    _prefs.end();
}

static void _save_networks() {
    _prefs.begin(WIFI_NAMESPACE, false);
    _prefs.putInt("count", _network_count);
    for (int i = 0; i < _network_count; i++) {
        char key_s[12], key_p[12];
        snprintf(key_s, sizeof(key_s), "ssid%d", i);
        snprintf(key_p, sizeof(key_p), "pwd%d",  i);
        _prefs.putString(key_s, _networks[i].ssid);
        _prefs.putString(key_p, _networks[i].pwd);
    }
    _prefs.end();
}

static void _try_connect() {
    if (_network_count == 0) return;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false);
    // Tente chaque réseau sauvegardé
    for (int i = 0; i < _network_count; i++) {
        if (_networks[i].ssid.isEmpty()) continue;
        Serial.printf("[WIFI] Connecting to %s...\n", _networks[i].ssid.c_str());
        WiFi.begin(_networks[i].ssid.c_str(), _networks[i].pwd.c_str());
        unsigned long t = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t < CONNECT_TIMEOUT_MS) {
            delay(200);
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[WIFI] Connected: %s IP=%s\n",
                _networks[i].ssid.c_str(), WiFi.localIP().toString().c_str());
            return;
        }
        WiFi.disconnect(false);
    }
    Serial.println("[WIFI] No network available");
}

void wifi_mgr_init() {
    _load_networks();
    _initialized = true;
    _try_connect();
}

void wifi_mgr_tick() {
    // Reconnexion automatique si connexion perdue
    static unsigned long _last_check = 0;
    if (!_initialized) return;
    unsigned long now = millis();
    if (now - _last_check < 15000) return;
    _last_check = now;
    if (WiFi.status() != WL_CONNECTED && _network_count > 0) {
        Serial.println("[WIFI] Lost connection, retrying...");
        _try_connect();
    }
}

void wifi_mgr_provision(const char* ssid, const char* pwd) {
    if (!ssid || !ssid[0]) return;
    // Vérifie si ce SSID est déjà sauvegardé
    for (int i = 0; i < _network_count; i++) {
        if (_networks[i].ssid == ssid) {
            _networks[i].pwd = pwd ? pwd : "";
            _save_networks();
            Serial.printf("[WIFI] Updated: %s\n", ssid);
            _try_connect();
            return;
        }
    }
    // Ajoute (ou écrase le plus ancien si plein)
    int slot = _network_count < MAX_NETWORKS ? _network_count++ : MAX_NETWORKS - 1;
    _networks[slot].ssid = ssid;
    _networks[slot].pwd  = pwd ? pwd : "";
    _save_networks();
    Serial.printf("[WIFI] Provisioned: %s\n", ssid);
    _try_connect();
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
