#pragma once
// ============================================================
// CompagnonV2 — HAL Audio I/O
// Codec : ES8311 (I2C config + I2S data)
// I2S   : SPK data GPIO 8, PA_EN GPIO 46
//         MIC BCLK GPIO 9, LRCK GPIO 45, DIN GPIO 10, MCLK GPIO 16
// Usage : TTS playback + capture mic pour STT / wake word
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include <driver/i2s_std.h>
#include "../config/pins.h"

// Paramètres audio
#define AUDIO_SAMPLE_RATE   16000   // Hz — Groq Whisper attend 16kHz
#define AUDIO_BITS          16      // bits/sample
#define AUDIO_CHANNELS_MIC  1       // mono mic
#define AUDIO_CHANNELS_SPK  1       // mono SPK

// Taille buffer I2S
#define AUDIO_DMA_BUF_COUNT 4
#define AUDIO_DMA_BUF_LEN   1024    // samples par buffer

/**
 * @brief Initialise le codec ES8311 (I2C) et les channels I2S (ESP-IDF i2s_std).
 *        À appeler dans setup() ou task_voice_io.
 */
bool hal_audio_init(void);

/**
 * @brief Lit des samples depuis le microphone.
 * @param buf    buffer de sortie (int16_t)
 * @param nsamples nombre de samples à lire
 * @return nombre de samples lus, -1 si erreur
 */
int  hal_audio_mic_read(int16_t* buf, size_t nsamples);

/**
 * @brief Joue des samples PCM sur le haut-parleur.
 * @param buf    buffer de samples (int16_t)
 * @param nsamples nombre de samples
 */
void hal_audio_spk_write(const int16_t* buf, size_t nsamples);

/**
 * @brief Joue un bip simple (pour notification rappel).
 * @param freq_hz  fréquence en Hz
 * @param dur_ms   durée en ms
 */
void hal_audio_beep(uint16_t freq_hz, uint16_t dur_ms);

/**
 * @brief Active/désactive le PA (amplificateur haut-parleur).
 */
void hal_audio_pa_enable(bool enable);

/**
 * @brief Retourne true si le codec ES8311 est initialisé.
 */
bool hal_audio_is_ready(void);
