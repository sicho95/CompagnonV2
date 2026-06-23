// ============================================================
// CompagnonV2 — net/ota.cpp
// fix: ArduinoOTA.begin() deplace dans loop() apres WiFi connected
//
// ArduinoOTA.begin() appelle MDNS.begin() + cree sockets UDP
// via lwIP qui prend des mutex internes (xQueueSemaphoreTake).
// Appele dans setup() sans WiFi actif -> mutex NULL -> crash.
//
// Solution : begin() appele UNE SEULE FOIS dans tick()
// quand WiFi.status() == WL_CONNECTED.
// ============================================================
#include "ota.h"
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <Arduino.h>

static bool _ota_started = false;

void net_ota_init() {
    // Callbacks seulement — pas de begin() ici
    ArduinoOTA.setHostname("CompagnonV2");
    ArduinoOTA.onStart([]()  { Serial.println("[OTA] Start"); });
    ArduinoOTA.onEnd([]()    { Serial.println("[OTA] End"); });
    ArduinoOTA.onError([](ota_error_t e) { Serial.printf("[OTA] Error %u\n", e); });
    Serial.println("[OTA] init OK (begin differe apres WiFi)");
}

void net_ota_tick() {
    if (!_ota_started) {
        if (WiFi.status() != WL_CONNECTED) return;
        ArduinoOTA.begin();
        _ota_started = true;
        Serial.println("[OTA] begin OK");
    }
    ArduinoOTA.handle();
}
