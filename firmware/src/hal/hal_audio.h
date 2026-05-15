#pragma once
#include <stdint.h>
#include <stddef.h>

// ============================================================
// CompagnonV2 — HAL Audio
// ES7210 (4-mic ADC) + I2S TX (speaker out)
// Arduino 3.3.8 — I2S legacy driver
// ============================================================

namespace hal {

// ── Init ───────────────────────────────────────────────────
// Call once after Wire.begin(SDA, SCL).
// Starts ES7210 at 16 kHz 16-bit TDM (MIC3+MIC4 = wake mics).
// Starts I2S TX at 16 kHz 16-bit mono for speaker.
bool audio_init();

// ── Microphone read (blocking, 16 kHz 16-bit S16_LE) ────────
// Returns actual samples read into buf.
size_t audio_mic_read(int16_t* buf, size_t samples);

// ── Speaker write (non-blocking via DMA) ────────────────────
// buf : PCM S16_LE mono 16 kHz
size_t audio_spk_write(const int16_t* buf, size_t samples);

// ── Speaker write raw bytes ──────────────────────────────────
size_t audio_spk_write_bytes(const uint8_t* buf, size_t len);

// ── Gain helpers ────────────────────────────────────────────
void audio_set_mic_gain(uint8_t gain_db);   // 0..37 dB

// ── Power ────────────────────────────────────────────────────
void audio_suspend();   // flush DMA + power-down ES7210
void audio_resume();    // restart

} // namespace hal
