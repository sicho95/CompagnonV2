#include "orchestrator.h"
#include <Arduino.h>

static bool _ready = false;

void orchestrator_init() {
    _ready = true;
    Serial.println("[ORCH] init OK");
}

void orchestrator_tick() {
    if (!_ready) return;
    // TODO: coordination apps / voice pipeline
}
