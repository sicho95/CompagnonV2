#pragma once
// ============================================================
// CompagnonV2 — hal/audio_io.h
// Audio I2S + codec ES8311
// Mic I2S (entrée ASR), SPK I2S via ES8311 + ampli PA
// Waveshare ESP32-S3 AMOLED 2.16"
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include "../config/pins.h"

#define ES8311_ADDR   0x18
#define AUDIO_SAMPLE_RATE   16000   // Hz — Whisper ASR
#define AUDIO_BITS          16
#define AUDIO_CHANNELS_MIC  1
#define AUDIO_CHANNELS_SPK  1

bool audio_init();

// MIC — capture PCM 16kHz mono pour wake word / ASR
bool audio_mic_start();
void audio_mic_stop();
int  audio_mic_read(int16_t* buf, size_t samples);  // retourne nb samples lus

// SPK — lecture PCM 16kHz mono pour TTS
bool audio_spk_start();
void audio_spk_stop();
bool audio_spk_play(const int16_t* buf, size_t samples);
void audio_spk_set_volume(uint8_t vol);  // 0-100

// Sons système
void audio_play_beep(uint16_t freq_hz, uint16_t duration_ms);
