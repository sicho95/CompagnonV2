#pragma once
#include <Arduino.h>
#include <functional>

// ============================================================
// CompagnonV2 — net/wifi_mgr.h
// Namespace WifiMgr — API publique
// ============================================================

using WifiConnectedCb    = std::function<void()>;
using WifiDisconnectedCb = std::function<void()>;

namespace WifiMgr {
    void setCallbacks(WifiConnectedCb onConnected, WifiDisconnectedCb onDisconnected);
    bool connect();
    void reconnect();
    bool isConnected();
    void tick();                // appeler depuis task_network toutes les ~5s
    void startAP();
    void saveCredentials(const String& ssid, const String& pass);
    time_t syncNtp();           // configTime + setenv(TZ) + retourne epoch UTC
}
