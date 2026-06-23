#include "ble_mgr.h"
#include <Arduino.h>

static ble_wifi_prov_cb_t   _wifi_cb    = nullptr;
static ble_agent_sync_cb_t  _agent_cb   = nullptr;

void ble_mgr_init() {
    // TODO: init BLE GATT server
    Serial.println("[BLE] init stub OK");
}
void ble_mgr_tick() {}
void ble_mgr_set_wifi_prov_cb(ble_wifi_prov_cb_t cb)   { _wifi_cb  = cb; }
void ble_mgr_set_agent_sync_cb(ble_agent_sync_cb_t cb)  { _agent_cb = cb; }
void ble_mgr_notify_agent_sync(const char* json) {
    Serial.printf("[BLE] notify: %s\n", json ? json : "");
}
