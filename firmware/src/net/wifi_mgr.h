// ============================================================
// CompagnonV2 — net/wifi_mgr.h
// ============================================================
#pragma once
#include <functional>
#include <Arduino.h>

namespace net {

void   wifi_init();
void   wifi_tick();
bool   wifi_is_connected();
bool   wifi_is_ap_mode();
time_t wifi_get_ntp_epoch();   // bloquant ~500ms, retourne 0 si échec
void   wifi_reconnect();       // force reconnexion STA (relit NVS)

void wifi_on_connected   (std::function<void()> cb);
void wifi_on_disconnected(std::function<void()> cb);

} // namespace net
