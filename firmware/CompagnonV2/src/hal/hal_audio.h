// ============================================================
// CompagnonV2 — hal_audio.h
// ES7210 ADC 4-mic (I2S RX TDM)  — capture micro / AEC
// ES8311 Codec DAC (I2S TX)       — lecture TTS Groq
// NS4150B PA                      — GPIO46 PA_EN
// ============================================================
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "drivers/es7210.h"
#include "drivers/es8311.h"

#define AUDIO_SAMPLE_RATE   16000
#define AUDIO_TTS_RATE      24000
#define AUDIO_CHANNELS      1
#define AUDIO_BITS          16

namespace hal {

bool   audio_init();
void   audio_suspend();
void   audio_resume();
void   audio_pa_enable(bool en);
void   audio_spk_enable(bool en);
size_t audio_mic_read(int16_t* buf, size_t samples);
void   audio_set_mic_gain(es7210_input_mic_t mic, es7210_gain_value_t gain);
void   audio_set_volume(uint8_t vol);
void   audio_play_pcm(const uint8_t* buf, size_t len);

} // namespace hal
