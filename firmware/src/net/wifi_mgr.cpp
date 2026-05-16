// ============================================================
// CompagnonV2 — net/wifi_mgr.cpp
// Namespace WifiMgr (B1 — était net::, harmonisé avec le .h)
// B2  — syncNtp() expose wifi_get_ntp_epoch() correctement
// Fix #4  — setenv(TZ) pour fuseau local
// Fix #6  — vTaskDelay dans retry loop
// Fix #11 — NTP timeout 500ms max
// W2  — tick() pour mise à jour état connexion
// ============================================================
#include "wifi_mgr.h"
#include "../storage/nvs_store.h"
#include <WiFi.h>
#include <esp_sntp.h>
#include <Arduino.h>

#define WIFI_RETRY_COUNT    10
#define WIFI_RETRY_DELAY_MS 1000
#define NTP_RETRY_COUNT     5
#define NTP_RETRY_DELAY_MS  100   // 5 × 100ms = 500ms max

namespace WifiMgr {

static bool _connected = false;
static bool _ap_mode   = false;

static WifiConnectedCb    _cb_connected;
static WifiDisconnectedCb _cb_disconnected;

void setCallbacks(WifiConnectedCb onConnected, WifiDisconnectedCb onDisconnected) {
    _cb_connected    = onConnected;
    _cb_disconnected = onDisconnected;
}

// ── Save credentials (depuis BLE / PWA) ─────────────────────
void saveCredentials(const String& ssid, const String& pass) {
    NvsStore::setString("wifi", "ssid", ssid);
    NvsStore::setString("wifi", "pass", pass);
}

// ── Fallback AP ───────────────────────────────────────────────
void startAP() {
    String ssid = NvsStore::getString("wifi", "ap_ssid", "Compagnon-AP");
    String pass = NvsStore::getString("wifi", "ap_pass", "compagnon2024");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid.c_str(), pass.c_str());
    _ap_mode = true;
    Serial.printf("[WIFI] AP mode: SSID=%s IP=%s\n",
        ssid.c_str(), WiFi.softAPIP().toString().c_str());
}

// ── Connexion STA ─────────────────────────────────────────────
bool connect() {
    String ssid = NvsStore::getString("wifi", "ssid", "");
    String pass = NvsStore::getString("wifi", "pass", "");

    if (ssid.isEmpty()) {
        Serial.println("[WIFI] No SSID configured → AP mode");
        startAP();
        return false;
    }

    Serial.printf("[WIFI] Connecting to %s\n", ssid.c_str());
    WiFi.mode(WIFI_STA);
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
        Serial.print(".");
    }

    Serial.println("\n[WIFI] Failed to connect → AP mode");
    startAP();
    return false;
}

// ── Reconnexion forcée (nouvelles credentials via BLE) ───────
void reconnect() {
    WiFi.disconnect(true);
    _connected = false;
    _ap_mode   = false;
    vTaskDelay(pdMS_TO_TICKS(200));
    connect();
}

// ── Tick (W2 — appeler depuis task_network) ──────────────────
void tick() {
    if (_ap_mode) return;
    bool now_connected = (WiFi.status() == WL_CONNECTED);
    if (!_connected && now_connected) {
        _connected = true;
        Serial.println("[WIFI] Reconnected");
        if (_cb_connected) _cb_connected();
    } else if (_connected && !now_connected) {
        _connected = false;
        Serial.println("[WIFI] Disconnected");
        if (_cb_disconnected) _cb_disconnected();
    }
}

bool isConnected() { return (_connected && WiFi.status() == WL_CONNECTED); }

// ── NTP sync (B2 — exposé publiquement, appelé après connexion)
// Fix #4 — timezone depuis NVS (défaut Paris CET/CEST)
// Fix #11 — timeout réduit : NTP_RETRY_COUNT × NTP_RETRY_DELAY_MS
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
