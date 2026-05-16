#include "voice_engine.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static bool _silent  = false;
static bool _running = false;

static void _voice_task(void* pvParam) {
    while (true) {
        if (!_silent) {
            // TODO: lecture mic ES7210, détection wake word
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void voice_engine_init() {
    xTaskCreatePinnedToCore(_voice_task, "voice", 4096, nullptr, 1, nullptr, 0);
    _running = true;
    Serial.println("[VOICE] engine init OK (Core 0)");
}
void voice_engine_set_silent(bool s) { _silent = s; }
bool voice_engine_is_listening()     { return _running && !_silent; }
