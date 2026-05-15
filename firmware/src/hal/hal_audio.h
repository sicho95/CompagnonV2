#pragma once
#include <stdint.h>
#include <stddef.h>

static constexpr uint32_t AUDIO_SAMPLE_RATE   = 16000;
static constexpr uint32_t AUDIO_FRAME_MS      = 30;
static constexpr size_t   AUDIO_FRAME_SAMPLES = (AUDIO_SAMPLE_RATE * AUDIO_FRAME_MS) / 1000;

namespace hal {
void   audio_init();
void   audio_mic_read(int16_t* buf, size_t n_samples);
void   audio_spk_write(const int16_t* buf, size_t n_samples);
void   audio_play_tone(uint16_t freq_hz, uint32_t duration_ms);
} // namespace hal
