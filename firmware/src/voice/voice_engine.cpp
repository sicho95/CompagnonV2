// ============================================================
// CompagnonV2 — VoiceEngine
// STT : Groq Whisper via HTTP multipart/form-data
// TTS : Groq PlayAI response_format=pcm → hal::audio_play_pcm()
// VAD : esp-sr vad_process
// FreeRTOS Core 0
// ============================================================
#include "voice_engine.h"
#include "../hal/hal_audio.h"
#include "../../include/pins.h"
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_vad.h>

#define GROQ_STT_URL  "https://api.groq.com/openai/v1/audio/transcriptions"
#define GROQ_TTS_URL  "https://api.groq.com/openai/v1/audio/speech"
#define SAMPLE_RATE   16000
#define FRAME_MS      30
#define FRAME_SAMPLES (SAMPLE_RATE * FRAME_MS / 1000)

namespace voice {

static Config       _cfg;
static SttCallback  _stt_cb;
static bool         _muted         = false;
static bool         _recording     = false;
static bool         _wake_detected = false;
static QueueHandle_t _tts_queue    = nullptr;
static TaskHandle_t  _task_handle  = nullptr;

// ── WAV header 44 bytes ─────────────────────────────────────
static void _wav_header(uint8_t* h, uint32_t pcm_bytes) {
    uint32_t total  = pcm_bytes + 36;
    uint32_t rate   = SAMPLE_RATE;
    uint16_t ch     = 1;
    uint16_t bits   = 16;
    uint32_t brate  = rate * ch * bits / 8;
    uint16_t balign = (uint16_t)(ch * bits / 8);
    memcpy(h,     "RIFF", 4); h += 4;
    memcpy(h,     &total,  4); h += 4;
    memcpy(h,     "WAVE", 4); h += 4;
    memcpy(h,     "fmt ", 4); h += 4;
    uint32_t sc = 16; memcpy(h, &sc, 4); h += 4;
    uint16_t af = 1;  memcpy(h, &af, 2); h += 2;
    memcpy(h, &ch,     2); h += 2;
    memcpy(h, &rate,   4); h += 4;
    memcpy(h, &brate,  4); h += 4;
    memcpy(h, &balign, 2); h += 2;
    memcpy(h, &bits,   2); h += 2;
    memcpy(h, "data",  4); h += 4;
    memcpy(h, &pcm_bytes, 4);
}

// ── STT Groq Whisper ─────────────────────────────────────────
static String _stt_groq(const int16_t* pcm, size_t samples) {
    uint32_t pcm_bytes = samples * sizeof(int16_t);
    uint8_t  wav_hdr[44];
    _wav_header(wav_hdr, pcm_bytes);
    const char* boundary = "CompagnonV2Boundary";
    String part1 = String("--") + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"audio.wav\"\r\n"
        "Content-Type: audio/wav\r\n\r\n";
    String part2 = String("\r\n--") + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"model\"\r\n\r\n"
        + _cfg.stt_model + "\r\n"
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"language\"\r\n\r\nfr\r\n"
        "--" + boundary + "--\r\n";
    size_t body_size = part1.length() + 44 + pcm_bytes + part2.length();
    uint8_t* body = (uint8_t*)ps_malloc(body_size);
    if (!body) return "";
    size_t off = 0;
    memcpy(body + off, part1.c_str(), part1.length()); off += part1.length();
    memcpy(body + off, wav_hdr, 44);                   off += 44;
    memcpy(body + off, pcm, pcm_bytes);                off += pcm_bytes;
    memcpy(body + off, part2.c_str(), part2.length());
    HTTPClient http; WiFiClientSecure client;
    client.setInsecure();
    http.begin(client, GROQ_STT_URL);
    http.addHeader("Authorization", String("Bearer ") + _cfg.groq_api_key);
    http.addHeader("Content-Type", String("multipart/form-data; boundary=") + boundary);
    http.addHeader("Content-Length", String(body_size));
    int code = http.POST(body, body_size);
    free(body);
    if (code != 200) { http.end(); return ""; }
    String resp = http.getString(); http.end();
    int ti = resp.indexOf("\"text\""); if (ti < 0) return "";
    int vi = resp.indexOf('"', ti + 7); if (vi < 0) return "";
    int ve = resp.indexOf('"', vi + 1); if (ve < 0) return "";
    return resp.substring(vi + 1, ve);
}

// ── TTS Groq PlayAI → hal::audio_play_pcm() ─────────────────
// FIX-ROUGE-2 : remplacé audio_spk_write_bytes() (inexistant)
// par accumulation dans buffer PSRAM + appel unique audio_play_pcm()
static void _tts_speak(const char* text) {
    if (_muted) return;
    HTTPClient http; WiFiClientSecure client;
    client.setInsecure();
    http.begin(client, GROQ_TTS_URL);
    http.addHeader("Authorization", String("Bearer ") + _cfg.groq_api_key);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(10000);
    String body = String("{") +
        "\"model\":\"" + _cfg.tts_model + "\"," +
        "\"voice\":\"" + _cfg.tts_voice + "\"," +
        "\"input\":\"" + String(text) + "\"," +
        "\"response_format\":\"pcm\"" + "}";
    int code = http.POST(body);
    if (code != 200) { http.end(); return; }

    // Accumulation en PSRAM puis lecture en une passe
    // (audio_play_pcm prend un buffer complet, pas un stream)
    const size_t MAX_TTS_BYTES = 64 * 1024;  // 64KB ~= 2s @ 16kHz 16bit
    uint8_t* pcm_buf = (uint8_t*)ps_malloc(MAX_TTS_BYTES);
    if (!pcm_buf) { http.end(); return; }
    size_t total = 0;
    WiFiClient* stream = http.getStreamPtr();
    while (http.connected() && total < MAX_TTS_BYTES) {
        int n = stream->readBytes(pcm_buf + total, MAX_TTS_BYTES - total);
        if (n <= 0) break;
        total += n;
    }
    http.end();
    if (total > 0) {
        hal::audio_play_pcm(pcm_buf, total);
    }
    free(pcm_buf);
}

// ── FreeRTOS task Core 0 ─────────────────────────────────────
static void _voice_task(void* pv) {
    vad_handle_t vad = vad_create(VAD_MODE_0);
    int16_t frame[FRAME_SAMPLES];
    const size_t MAX_PCM = SAMPLE_RATE * 10;
    int16_t* rec_buf = (int16_t*)ps_malloc(MAX_PCM * sizeof(int16_t));
    size_t   rec_idx = 0;
    uint32_t silence_start = 0;
    bool     in_speech = false;
    for (;;) {
        if (!_recording) {
            char* tts_text = nullptr;
            if (xQueueReceive(_tts_queue, &tts_text, 0) == pdTRUE) {
                _tts_speak(tts_text); free(tts_text); continue;
            }
        }
        if (_muted && !_recording) { vTaskDelay(pdMS_TO_TICKS(30)); continue; }
        hal::audio_mic_read(frame, FRAME_SAMPLES);
        vad_state_t vs = vad_process(vad, frame, SAMPLE_RATE, FRAME_MS);
        if (!_recording) {
            if (vs == VAD_SPEECH) {
                _wake_detected = true; _recording = true;
                rec_idx = 0; in_speech = true; silence_start = millis();
            }
            continue;
        }
        if (rec_idx + FRAME_SAMPLES <= MAX_PCM) {
            memcpy(rec_buf + rec_idx, frame, FRAME_SAMPLES * sizeof(int16_t));
            rec_idx += FRAME_SAMPLES;
        }
        if (vs == VAD_SPEECH) { in_speech = true; silence_start = millis(); }
        else if (in_speech) {
            if (millis() - silence_start > _cfg.vad_silence_ms ||
                rec_idx * 1000 / SAMPLE_RATE >= _cfg.max_record_ms) {
                _recording = false;
                String text = _stt_groq(rec_buf, rec_idx);
                if (text.length() > 0 && _stt_cb) _stt_cb(text.c_str());
                in_speech = false; rec_idx = 0;
            }
        }
    }
    vad_destroy(vad); free(rec_buf); vTaskDelete(NULL);
}

void init(const Config& cfg, SttCallback cb) {
    _cfg = cfg; _stt_cb = cb;
    _tts_queue = xQueueCreate(4, sizeof(char*));
    // FreeRTOS task interne sur Core 0 — stack 8192 (inchangée, suffisante)
    xTaskCreatePinnedToCore(_voice_task, "voice_io", 8192, nullptr, 5, &_task_handle, 0);
}

bool start_recording() { if (_recording) return false; _recording = true; return true; }
void stop_recording()  { _recording = false; }
void speak(const char* text) {
    if (!text || !*text) return;
    char* copy = strdup(text);
    if (xQueueSend(_tts_queue, &copy, pdMS_TO_TICKS(200)) != pdTRUE) free(copy);
}
bool wake_word_detected() { return _wake_detected; }
void clear_wake_word()    { _wake_detected = false; }
void set_mute(bool m)     { _muted = m; }
bool is_muted()           { return _muted; }

} // namespace voice
