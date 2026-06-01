// ============================================================
// CompagnonV2 — net/wifi_mgr.cpp
// Sans SSID : tente AP une fois, sans retry.
// ============================================================
#include "wifi_mgr.h"
#include "../storage/nvs_store.h"
#include <WiFi.h>
#include <esp_sntp.h>
#include <Arduino.h>

#define WIFI_RETRY_COUNT    10
#define WIFI_RETRY_DELAY_MS 1000
#define NTP_RETRY_COUNT     5
#define NTP_RETRY_DELAY_MS  100

namespace WifiMgr {

static bool _connected  = false;
static bool _ap_mode    = false;
static bool _wifi_up    = false;

static WifiConnectedCb    _cb_connected;
static WifiDisconnectedCb _cb_disconnected;

void setCallbacks(WifiConnectedCb onConnected, WifiDisconnectedCb onDisconnected) {
    _cb_connected    = onConnected;
    _cb_disconnected = onDisconnected;
}

void saveCredentials(const String& ssid, const String& pass) {
    NvsStore::setString("wifi", "ssid", ssid);
    NvsStore::setString("wifi", "pass", pass);
}

static bool _startAP() {
    String ssid = NvsStore::getString("wifi", "ap_ssid", "Compagnon-AP");
    String pass = NvsStore::getString("wifi", "ap_pass", "compagnon2024");
    if (!WiFi.mode(WIFI_AP)) {
        Serial.println("[WIFI] AP: WiFi.mode(WIFI_AP) failed — skip");
        return false;
    }
    if (!WiFi.softAP(ssid.c_str(), pass.c_str())) {
        Serial.println("[WIFI] AP: softAP failed — skip");
        return false;
    }
    _ap_mode = true;
    _wifi_up = true;
    Serial.printf("[WIFI] AP mode: SSID=%s IP=%s\n",
        ssid.c_str(), WiFi.softAPIP().toString().c_str());
    return true;
}

bool connect() {
    String ssid = NvsStore::getString("wifi", "ssid", "");
    String pass = NvsStore::getString("wifi", "pass", "");
    if (ssid.isEmpty()) {
        Serial.println("[WIFI] No SSID — skip WiFi entirely");
        return false;  // pas d'AP, pas de retry, rien
    }
    Serial.printf("[WIFI] Connecting to %s\n", ssid.c_str());
    if (!WiFi.mode(WIFI_STA)) {
        Serial.println("[WIFI] WiFi.mode(WIFI_STA) failed");
        return false;
    }
    _wifi_up = true;
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid.c_str(), pass.c_str());
    for (int i = 0; i < WIFI_RETRY_COUNT; i++) {
        if (WiFi.status() == WL_CONNECTED) {
            _connected = true;
            _ap_mode   = false;
            Serial.printf("[WIFI] Connected — IP: %s\n",
                WiFi.localIP().toString().c_str());
            if (_cb_connected) _cb_connected();
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(WIFI_RETRY_DELAY_MS));
    }
    Serial.println("[WIFI] Failed to connect — AP fallback");
    _startAP();
    return false;
}

void reconnect() {
    if (!_wifi_up) return;
    WiFi.disconnect(true);
    _connected = false;
    _ap_mode   = false;
    _wifi_up   = false;
    vTaskDelay(pdMS_TO_TICKS(500));
    connect();
}

void tick() {
    if (!_wifi_up || _ap_mode) return;
    bool now_connected = (WiFi.status() == WL_CONNECTED);
    if (!_connected && now_connected) {
        _connected = true;
        if (_cb_connected) _cb_connected();
    } else if (_connected && !now_connected) {
        _connected = false;
        if (_cb_disconnected) _cb_disconnected();
    }
}

bool isConnected() { return (_connected && WiFi.status() == WL_CONNECTED); }

time_t syncNtp() {
    if (!isConnected()) return 0;
    configTime(0, 0, "pool.ntp.org", "time.google.com");
    String tz = NvsStore::getString("system", "timezone",
                                    "CET-1CEST,M3.5.0,M10.5.0/3");
    setenv("TZ", tz.c_str(), 1);
    tzset();
    struct tm ti;
    for (int i = 0; i < NTP_RETRY_COUNT; i++) {
        if (getLocalTime(&ti, NTP_RETRY_DELAY_MS)) {
            time_t epoch = mktime(&ti);
            Serial.printf("[WIFI] NTP epoch: %lu (TZ: %s)\n",
                (unsigned long)epoch, tz.c_str());
            return epoch;
        }
    }
    Serial.println("[WIFI] NTP timeout");
    return 0;
}

} // namespace WifiMgr
