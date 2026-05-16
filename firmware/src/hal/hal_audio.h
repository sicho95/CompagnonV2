// ============================================================
// CompagnonV2 — hal_audio.h
// ES7210 ADC 4-mic (I2S RX TDM) + NS4150B ampli analogique
// audio_play_pcm : lecture PCM 16-bit via DAC interne ESP32-S3
// ============================================================
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "../../../lib/es7210/es7210.h"

#define AUDIO_SAMPLE_RATE   16000
#define AUDIO_CHANNELS      1
#define AUDIO_BITS          16

namespace hal {

bool   audio_init();
void   audio_suspend();
void   audio_resume();
void   audio_pa_enable(bool en);
size_t audio_mic_read(int16_t* buf, size_t samples);
void   audio_spk_enable(bool en);
void   audio_set_mic_gain(es7210_input_mic_t mic, es7210_gain_value_t gain);

// Lecture PCM Groq TTS → DAC interne ESP32-S3 (I2S TX)
// buf : données PCM 16-bit signed, len : nombre d'octets
void   audio_play_pcm(const uint8_t* buf, size_t len);

} // namespace hal
