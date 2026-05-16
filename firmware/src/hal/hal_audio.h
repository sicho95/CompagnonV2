// ============================================================
// CompagnonV2 — hal_audio.h
// ES7210 ADC 4-mic (I2S RX TDM)  — capture micro / AEC
// ES8311 Codec DAC (I2S TX)       — lecture TTS Groq
// NS4150B PA                      — GPIO46 PA_EN
// ============================================================
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "../../../lib/es7210/es7210.h"
#include "../../../lib/es8311/es8311.h"

#define AUDIO_SAMPLE_RATE   16000   // capture micro ES7210
#define AUDIO_TTS_RATE      24000   // lecture TTS ES8311
#define AUDIO_CHANNELS      1
#define AUDIO_BITS          16

namespace hal {

// Init complète : ES7210 + ES8311 + I2S RX + I2S TX
bool   audio_init();

// Suspend / resume (deep sleep)
void   audio_suspend();
void   audio_resume();

// PA ampli
void   audio_pa_enable(bool en);
void   audio_spk_enable(bool en);   // alias

// Capture micro ES7210
size_t audio_mic_read(int16_t* buf, size_t samples);
void   audio_set_mic_gain(es7210_input_mic_t mic, es7210_gain_value_t gain);

// Volume ES8311 DAC : 0 (muet) … 255 (max, 0dB = 0xBF)
void   audio_set_volume(uint8_t vol);

// Lecture PCM Groq TTS → ES8311 DAC → NS4150B → speaker
// buf : PCM 16-bit signed (WAV header détecté et skippé auto)
// len : taille en octets
void   audio_play_pcm(const uint8_t* buf, size_t len);

} // namespace hal
