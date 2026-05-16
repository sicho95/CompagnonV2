#include "voice_engine.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

static bool          _silent  = false;
static bool          _running = false;
static QueueHandle_t _speak_q = nullptr;

#define SPEAK_MAX_LEN 256

static void _voice_task(void* pvParam) {
    char msg[SPEAK_MAX_LEN];
    while (true) {
        if (xQueueReceive(_speak_q, msg, pdMS_TO_TICKS(20)) == pdTRUE) {
            if (!_silent) {
                Serial.printf("[VOICE] TTS: %s\n", msg);
                // TODO : appeler hal::audio_play_pcm( HttpClient::textToSpeech(msg) )
            }
        }
        // TODO : lecture mic ES7210, détection wake word
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void voice_engine_init() {
    _speak_q = xQueueCreate(4, SPEAK_MAX_LEN);
    xTaskCreatePinnedToCore(_voice_task, "voice", 8192, nullptr, 1, nullptr, 0);
    _running = true;
    Serial.println("[VOICE] engine init OK (Core 0)");
}

void voice_engine_set_silent(bool s) { _silent = s; }
bool voice_engine_is_listening()     { return _running && !_silent; }

void voice_engine_speak(const char* text) {
    if (!text || !_speak_q) return;
    char msg[SPEAK_MAX_LEN];
    strncpy(msg, text, SPEAK_MAX_LEN - 1);
    msg[SPEAK_MAX_LEN - 1] = 0;
    xQueueSend(_speak_q, msg, 0);  // non-bloquant
}
