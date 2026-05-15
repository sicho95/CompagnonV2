// ============================================================
// voice_engine.cpp — Pipeline vocal CompagnonV2
// Core 0 : VAD + capture mic (task_voice_io)
// Core 1 : consommateur STT (task_stt_consumer)
//
// ESP-SR WakeNet : blocs à décommenter dès que la lib
//   espressif/esp-sr est disponible sous Arduino 3.3.8
//
// STT Groq Whisper : squelette WAV multipart (TODO)
// TTS Groq PlayAI  : squelette HTTP stream PCM  (TODO)
// ============================================================
#include "voice_engine.h"
#include "../hal/hal_audio.h"
#include <esp_vad.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <Preferences.h>
#include <Arduino.h>

// #include <esp_wn_iface.h>
// #include <esp_wn_models.h>
// static esp_wakeword_iface_t* _wn       = NULL;
// static model_iface_data_t*   _wn_model = NULL;

namespace voice {

static WakeCallback  _on_wake      = nullptr;
static SttCallback   _on_stt       = nullptr;
static TaskHandle_t  _task_voice   = nullptr;
static QueueHandle_t _stt_q        = nullptr;
static volatile bool _stt_trigger  = false;
static volatile bool _silent       = false;
static Preferences   _prefs;

static int16_t* _vad_buf = nullptr;
static int16_t* _stt_buf = nullptr;
static const size_t STT_MAX_SAMPLES = AUDIO_SAMPLE_RATE * 6;

// ── WAV header 44 bytes ──────────────────────────────────────
static void _write_wav_header(uint8_t* h, uint32_t pcm_bytes,
                              uint32_t sr, uint16_t ch) {
    uint32_t data_size   = pcm_bytes;
    uint32_t riff_size   = 36 + data_size;
    uint32_t byte_rate   = sr * ch * 2;
    uint16_t block_align = ch * 2;
    uint16_t bps         = 16;
    uint16_t pcm1        = 1;
    uint32_t fmt16       = 16;
    memcpy(h,    "RIFF", 4);  memcpy(h+4,  &riff_size,   4);
    memcpy(h+8,  "WAVE", 4);
    memcpy(h+12, "fmt ", 4);  memcpy(h+16, &fmt16,       4);
    memcpy(h+20, &pcm1,  2);  memcpy(h+22, &ch,          2);
    memcpy(h+24, &sr,    4);  memcpy(h+28, &byte_rate,   4);
    memcpy(h+32, &block_align, 2); memcpy(h+34, &bps, 2);
    memcpy(h+36, "data", 4);  memcpy(h+40, &data_size,   4);
}

// ── STT Groq Whisper (squelette) ─────────────────────────────
static void stt_groq(const int16_t* buf, size_t n_samples) {
    Preferences p;
    p.begin("api_keys", true);
    String key = p.getString("groq", "");
    p.end();
    if (key.isEmpty()) {
        Serial.println("[STT] Cle Groq manquante — configurer depuis PWA");
        return;
    }
    // TODO: construire WAV, POST multipart vers
    //   https://api.groq.com/openai/v1/audio/transcriptions
    // et appeler _on_stt(texte_transcrit)
    Serial.printf("[STT] TODO Groq Whisper — %u samples\n", (unsigned)n_samples);
}

// ── TTS Groq PlayAI (squelette) ──────────────────────────────
void tts_speak(const String& text) {
    if (_silent || text.isEmpty()) return;
    Preferences p;
    p.begin("api_keys", true);
    String key = p.getString("groq", "");
    p.end();
    if (key.isEmpty()) { Serial.println("[TTS] Cle Groq manquante"); return; }
    // TODO: POST JSON vers https://api.groq.com/openai/v1/audio/speech
    //   { model:playai-tts, voice:Celeste-PlayAI, input:text, response_format:pcm }
    // streamer le PCM vers hal::audio_spk_write
    Serial.printf("[TTS] TODO PlayAI → '%s'\n", text.c_str());
}

// ── Tache Core 0 ─────────────────────────────────────────────
static void _voice_task(void*) {
    vad_handle_t vad = vad_create(VAD_MODE_0);
    bool   recording   = false;
    size_t stt_samples = 0;
    uint32_t silence_ms = 0;

    for (;;) {
        hal::audio_mic_read(_vad_buf, AUDIO_FRAME_SAMPLES);
        vad_state_t vs = vad_process(vad, _vad_buf, AUDIO_SAMPLE_RATE, AUDIO_FRAME_MS);

        // Wake word ESP-SR — decommenter quand lib dispo
        // int wr = _wn->detect(_wn_model, _vad_buf);
        // if (wr > 0) { if (_on_wake) _on_wake(wr); recording=true; stt_samples=0; }

        if (_stt_trigger) {
            _stt_trigger = false; recording = true; stt_samples = 0; silence_ms = 0;
        }

        if (recording && stt_samples < STT_MAX_SAMPLES) {
            size_t copy = min((size_t)AUDIO_FRAME_SAMPLES, STT_MAX_SAMPLES - stt_samples);
            memcpy(_stt_buf + stt_samples, _vad_buf, copy * sizeof(int16_t));
            stt_samples += copy;
            silence_ms = (vs == VAD_SILENCE) ? silence_ms + AUDIO_FRAME_MS : 0;
            bool min_ok = stt_samples > AUDIO_SAMPLE_RATE;
            if ((min_ok && silence_ms >= 800) || stt_samples >= STT_MAX_SAMPLES) {
                recording = false;
                xQueueSend(_stt_q, (void*)&stt_samples, 0);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    vad_destroy(vad);
}

static void _stt_consumer_task(void*) {
    size_t n;
    for (;;) {
        if (xQueueReceive(_stt_q, &n, portMAX_DELAY) == pdTRUE)
            stt_groq(_stt_buf, n);
    }
}

bool voice_init(WakeCallback on_wake, SttCallback on_stt) {
    _on_wake = on_wake;  _on_stt = on_stt;
    _vad_buf = (int16_t*)ps_malloc(AUDIO_FRAME_SAMPLES * sizeof(int16_t));
    _stt_buf = (int16_t*)ps_malloc(STT_MAX_SAMPLES     * sizeof(int16_t));
    if (!_vad_buf || !_stt_buf) { Serial.println("[voice] PSRAM insuffisante"); return false; }
    _stt_q = xQueueCreate(2, sizeof(size_t));
    _prefs.begin("os_prefs", true);
    _silent = _prefs.getBool("silent", false);
    _prefs.end();
    return true;
}
void voice_start_task() {
    xTaskCreatePinnedToCore(_voice_task, "voice_io", 8192, NULL, 5, &_task_voice, 0);
    xTaskCreatePinnedToCore(_stt_consumer_task, "stt_consumer", 8192, NULL, 3, NULL, 1);
}
void voice_stop_task() {
    if (_task_voice) { vTaskDelete(_task_voice); _task_voice = nullptr; }
}
void voice_trigger_stt() { _stt_trigger = true; }
void voice_set_silent(bool s) {
    _silent = s;
    _prefs.begin("os_prefs", false); _prefs.putBool("silent", s); _prefs.end();
}
bool voice_is_silent() { return _silent; }

} // namespace voice
