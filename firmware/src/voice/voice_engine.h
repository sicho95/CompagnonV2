#pragma once
#include <stdint.h>
#include <stddef.h>
#include <functional>

// ============================================================
// CompagnonV2 — VoiceEngine
// Pipeline : VAD → STT (Groq Whisper) → callback(text)
//          : TTS (Groq PlayAI) → hal::audio_spk_write_bytes()
// Runs on Core 0 via FreeRTOS task.
// ============================================================

namespace voice {

// ── STT result callback ─────────────────────────────────────
// Called on Core 0 ; post to main queue if you touch LVGL.
using SttCallback = std::function<void(const char* text)>;

// ── Config ──────────────────────────────────────────────────
struct Config {
    const char* groq_api_key    = nullptr;  // from NVS
    const char* stt_model       = "whisper-large-v3-turbo"; // W1 — défaut corrigé
    const char* tts_model       = "playai-tts";
    const char* tts_voice       = "Fritz-PlayAI";   // FR-friendly
    uint32_t    vad_silence_ms  = 1200;   // end-of-speech timeout
    uint32_t    max_record_ms   = 10000;  // safety cap
    bool        use_ble_relay   = false;  // fallback HTTP via PWA
};

void init(const Config& cfg, SttCallback cb);
bool start_recording();
void stop_recording();
void speak(const char* text);
bool wake_word_detected();
void clear_wake_word();
void set_mute(bool muted);
bool is_muted();

} // namespace voice
