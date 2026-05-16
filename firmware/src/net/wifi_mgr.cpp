// ============================================================
// CompagnonV2 — net/wifi_mgr.cpp
// WiFi STA avec retry → fallback AP
// NTP sync exposée via wifi_get_ntp_epoch()
// Credentials depuis NVS (namespace "wifi")
// ============================================================
#include "wifi_mgr.h"
#include "../storage/nvs_store.h"
#include <WiFi.h>
#include <esp_sntp.h>
#include <Arduino.h>

#define WIFI_RETRY_COUNT  10
#define WIFI_RETRY_DELAY_MS 1000

namespace net {

static bool _connected    = false;
static bool _ap_mode      = false;
static bool _ntp_synced   = false;
static uint32_t _last_check_ms = 0;

// ── Callbacks optionnels ──────────────────────────────────────
std::function<void()> _cb_connected;
std::function<void()> _cb_disconnected;

void wifi_on_connected   (std::function<void()> cb) { _cb_connected    = cb; }
void wifi_on_disconnected(std::function<void()> cb) { _cb_disconnected = cb; }

// ── Fallback AP ───────────────────────────────────────────────
static void _start_ap() {
    String ssid = NvsStore::getString("wifi", "ap_ssid", "Compagnon-AP");
    String pass = NvsStore::getString("wifi", "ap_pass", "compagnon2024");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid.c_str(), pass.c_str());
    _ap_mode = true;
    Serial.printf("[WIFI] AP mode: SSID=%s IP=%s\n",
        ssid.c_str(), WiFi.softAPIP().toString().c_str());
}

// ── Init ──────────────────────────────────────────────────────
void wifi_init() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    String ssid = NvsStore::getString("wifi", "ssid", "");
    String pass = NvsStore::getString("wifi", "pass", "");

    if (ssid.isEmpty()) {
        Serial.println("[WIFI] No SSID configured → AP mode");
        _start_ap();
        return;
    }

    Serial.printf("[WIFI] Connecting to %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), pass.c_str());

    for (int i = 0; i < WIFI_RETRY_COUNT; i++) {
        if (WiFi.status() == WL_CONNECTED) {
            _connected = true;
            Serial.printf("[WIFI] Connected — IP: %s\n",
                WiFi.localIP().toString().c_str());
            if (_cb_connected) _cb_connected();
            return;
        }
        delay(WIFI_RETRY_DELAY_MS);
        Serial.print(".");
    }

    Serial.println("\n[WIFI] Failed to connect → AP mode");
    _start_ap();
}

// ── Tick (appelé depuis task_network toutes les 1s) ───────────
void wifi_tick() {
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

bool wifi_is_connected() {
    return (_connected && WiFi.status() == WL_CONNECTED);
}

bool wifi_is_ap_mode() {
    return _ap_mode;
}

// ── NTP sync (bloquant ~500ms, appelé UNE FOIS depuis task_network) ──
time_t wifi_get_ntp_epoch() {
    if (!wifi_is_connected()) return 0;
    configTime(0, 0, "pool.ntp.org", "time.google.com");
    // Attendre une sync valide (max 5s)
    struct tm ti;
    for (int i = 0; i < 10; i++) {
        if (getLocalTime(&ti, 500)) {
            time_t epoch = mktime(&ti);
            Serial.printf("[WIFI] NTP epoch: %lu\n", (unsigned long)epoch);
            return epoch;
        }
    }
    Serial.println("[WIFI] NTP timeout");
    return 0;
}

} // namespace net
