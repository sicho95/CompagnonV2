#pragma once
#include <Arduino.h>
#include <functional>

namespace ble {

using TextInputCb  = std::function<void(const String&)>;
using AgentSyncCb  = std::function<void(const String&)>;
using LlmRelayCb   = std::function<void(const String&)>;

void ble_init(TextInputCb on_text,
              AgentSyncCb on_agent,
              LlmRelayCb  on_llm);

void ble_notify_status       (const String& json);
void ble_notify_gps          (float lat, float lon, float alt);
void ble_notify_agent_sync   (const String& json);
void ble_notify_llm_relay    (const String& json);
void ble_set_wifi_scan_result(const String& json);
bool ble_connected           ();

} // namespace ble
