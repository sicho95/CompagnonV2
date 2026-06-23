#pragma once
#include <Arduino.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ble_wifi_prov_cb_t)(const char* json);
typedef void (*ble_agent_sync_cb_t)(const char* json);

void ble_mgr_init();
void ble_mgr_tick();
void ble_mgr_set_wifi_prov_cb(ble_wifi_prov_cb_t cb);
void ble_mgr_set_agent_sync_cb(ble_agent_sync_cb_t cb);
void ble_mgr_notify_agent_sync(const char* json);

#ifdef __cplusplus
}
#endif
