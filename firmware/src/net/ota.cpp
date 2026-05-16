#include "ota.h"
#include <ArduinoOTA.h>
#include <Arduino.h>

void net_ota_init() {
    ArduinoOTA.setHostname("CompagnonV2");
    ArduinoOTA.onStart([]()  { Serial.println("[OTA] Start"); });
    ArduinoOTA.onEnd([]()    { Serial.println("[OTA] End"); });
    ArduinoOTA.onError([](ota_error_t e) { Serial.printf("[OTA] Error %u\n", e); });
    ArduinoOTA.begin();
    Serial.println("[OTA] init OK");
}

void net_ota_tick() { ArduinoOTA.handle(); }
