// ============================================================
// CompagnonV2 — hal_audio.h
// ES7210 ADC 4-mic + NS4150B analogique
// Pas de I2S TX : le DAC ES7210 alimente directement le NS4150B
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
void   audio_pa_enable(bool en);           // GPIO46 NS4150B CTRL
size_t audio_mic_read(int16_t* buf, size_t samples);
void   audio_spk_enable(bool en);          // alias pa_enable pour clarté
void   audio_set_mic_gain(es7210_input_mic_t mic, es7210_gain_value_t gain);

} // namespace hal
