#include "voice_engine.h"
#include "../hal/audio.h"
#include "../net/ble_mgr.h"
#include "../config/nvs_config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <string.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// NOTE : remplacer par l'include ESP-SR réel quand disponible dans l'env Arduino
// #include "esp_wn_iface.h"
// #include "esp_wn_models.h"

static const char *TAG = "VOICE";

static bool                 s_silent       = false;
static bool                 s_listen_req   = false;  // bouton micro
static voice_dispatch_cb_t  s_dispatch_cb  = nullptr;
static char                 s_wake_word[32] = "nestor";
static TaskHandle_t         s_task_handle  = nullptr;

// ─── STT via Groq Whisper ─────────────────────────────────────────────────────
static bool stt_groq(const int16_t *pcm, size_t samples, char *out_text, size_t out_len) {
    // TODO : encoder en WAV/PCM et envoyer à https://api.groq.com/openai/v1/audio/transcriptions
    // Placeholder — retourne faux si pas implémenté
    ESP_LOGW(TAG, "stt_groq: stub — a implementer");
    (void)pcm; (void)samples;
    strncpy(out_text, "", out_len);
    return false;
}

// ─── STT via BLE relay (fallback sans WiFi) ──────────────────────────────────
static bool stt_ble_relay(const int16_t *pcm, size_t samples, char *out_text, size_t out_len) {
    // TODO : sérialiser PCM en base64 + envoyer via LLM_RELAY BLE
    ESP_LOGW(TAG, "stt_ble_relay: stub — a implementer");
    (void)pcm; (void)samples;
    strncpy(out_text, "", out_len);
    return false;
}

// ─── TTS via Groq PlayAI ─────────────────────────────────────────────────────
static void tts_speak(const char *text) {
    if (s_silent || !text || !text[0]) return;
    ESP_LOGI(TAG, "TTS: %s", text);
    // TODO : POST à https://api.groq.com/openai/v1/audio/speech
    //        puis lire le stream PCM et appeler hal_audio_play()
    //        Fallback BLE relay si pas de WiFi
}

// ─── Tâche wake word (Core 0) ────────────────────────────────────────────────
static void voice_task(void *arg) {
    ESP_LOGI(TAG, "Task demarree sur Core %d", xPortGetCoreID());

    // Buffer de capture (320 samples @ 16kHz = 20 ms)
    const size_t FRAME_SAMPLES = 320;
    int16_t frame[FRAME_SAMPLES];
    char    recognized[256];

    while (true) {
        bool triggered = false;

        // ── Demande manuelle (bouton micro LVGL) ──────────────────────────────
        if (s_listen_req) {
            s_listen_req = false;
            triggered    = true;
        }

        // ── Wake word : lire une frame et analyser ────────────────────────────
        // TODO : brancher ESP-SR WakeNet ici
        // int r = hal_audio_read(frame, FRAME_SAMPLES);
        // if (r > 0) { if (wakenet->detect(model_data, frame) > 0) triggered = true; }

        if (triggered) {
            ESP_LOGI(TAG, "Wake / bouton detecte — capture STT");
            // Capture 3 secondes
            const size_t RECORD_SAMPLES = SAMPLE_RATE * 3;  // 3 s
            int16_t *rec_buf = (int16_t *)malloc(RECORD_SAMPLES * sizeof(int16_t));
            if (!rec_buf) { vTaskDelay(pdMS_TO_TICKS(10)); continue; }

            size_t total = 0;
            while (total < RECORD_SAMPLES) {
                int got = hal_audio_read(rec_buf + total,
                                        RECORD_SAMPLES - total < FRAME_SAMPLES
                                            ? RECORD_SAMPLES - total
                                            : FRAME_SAMPLES);
                if (got > 0) total += got;
                vTaskDelay(pdMS_TO_TICKS(1));
            }

            bool ok = false;
            if (WiFi.isConnected())
                ok = stt_groq(rec_buf, total, recognized, sizeof(recognized));
            if (!ok)
                ok = stt_ble_relay(rec_buf, total, recognized, sizeof(recognized));

            free(rec_buf);

            if (ok && recognized[0] && s_dispatch_cb) {
                ESP_LOGI(TAG, "Reconnu: '%s'", recognized);
                s_dispatch_cb(recognized, "voice");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ─── API publique ─────────────────────────────────────────────────────────────
void voice_engine_init(void) {
    s_silent    = nvs_get_bool("silent_mode", false);
    xTaskCreatePinnedToCore(voice_task, "voice_task", 8192, nullptr,
                            5, &s_task_handle, 0);  // Core 0
    ESP_LOGI(TAG, "Voice engine init OK (silent=%s)", s_silent ? "oui" : "non");
}

void voice_engine_deinit(void) {
    if (s_task_handle) { vTaskDelete(s_task_handle); s_task_handle = nullptr; }
}

void voice_engine_start_listen(void) { s_listen_req = true; }

void voice_engine_speak(const char *text) { tts_speak(text); }

void voice_engine_set_silent(bool silent) {
    s_silent = silent;
    ESP_LOGI(TAG, "Mode silencieux: %s", silent ? "ON" : "OFF");
}

bool voice_engine_is_silent(void) { return s_silent; }

void voice_engine_set_wake_word(const char *word) {
    if (word && word[0]) strncpy(s_wake_word, word, sizeof(s_wake_word) - 1);
}

void voice_engine_set_dispatch_cb(voice_dispatch_cb_t cb) { s_dispatch_cb = cb; }
