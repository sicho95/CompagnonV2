#pragma once
// =============================================================
// CompagnonV2 — hal/audio_io.h
// Codec entrée : ES7210 (4-mic array, I2S)
// Codec sortie : ES8311 (DAC mono/stéréo, I2S partagé)
// Ampli        : PA enable GPIO 46
// =============================================================
#include <Wire.h>
#include <driver/i2s.h>
#include "../config/pins.h"

// Port I2S utilisé
#define AUDIO_I2S_PORT       I2S_NUM_1
#define AUDIO_SAMPLE_RATE    16000
#define AUDIO_BITS           I2S_BITS_PER_SAMPLE_16BIT
#define AUDIO_DMA_BUF_COUNT  8
#define AUDIO_DMA_BUF_LEN    512

/**
 * @brief Initialise le bus I2S, configure ES7210 (entrée) et ES8311 (sortie)
 *        via I2C. Active l'ampli PA.
 * @return true si les deux codecs répondent sur I2C.
 */
bool audio_init();

/**
 * @brief Lit des échantillons PCM 16 bits depuis le micro ES7210.
 * @param buf       Buffer de destination.
 * @param buf_bytes Taille en octets du buffer.
 * @param[out] bytes_read Octets effectivement lus.
 */
void audio_read(int16_t *buf, size_t buf_bytes, size_t *bytes_read);

/**
 * @brief Envoie des échantillons PCM 16 bits vers le DAC ES8311.
 * @param buf        Buffer source.
 * @param buf_bytes  Taille en octets.
 * @param[out] bytes_written Octets écrits.
 */
void audio_write(const int16_t *buf, size_t buf_bytes, size_t *bytes_written);

/** @brief Active ou désactive l'amplificateur PA. */
void audio_set_pa(bool enable);

/** @brief Règle le volume de sortie ES8311 (0–100). */
void audio_set_volume(uint8_t vol_pct);
