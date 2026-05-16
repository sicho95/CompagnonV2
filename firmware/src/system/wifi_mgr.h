#pragma once
// ============================================================
// WiFi Manager — stub public API (implémentation dans wifi_mgr.cpp)
// ============================================================
#include <Arduino.h>
#ifdef __cplusplus
extern "C" {
#endif

void    wifi_mgr_init();
void    wifi_mgr_tick();
void    wifi_mgr_provision(const char* ssid, const char* pwd);
void    wifi_mgr_remove_network(const char* ssid);
String  wifi_mgr_list_networks();
bool    wifi_mgr_is_connected();

#ifdef __cplusplus
}
#endif
