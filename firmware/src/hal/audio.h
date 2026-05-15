#pragma once
/*
 * hal/audio.h — Gestion I2S microphone + codec (CompagnonV2)
 *
 * Carte : Waveshare ESP32-S3-Touch-AMOLED-2.16
 * Mic   : INMP441 ou PDM selon variante (I2S0)
 * Codec : NS4168 / MAX98357 pour sortie audio (I2S1)
 *
 * Usage :
 *   hal_audio_init()          — init I2S mic + codec
 *   hal_audio_read(buf, len)  — lit len samples PCM 16-bit depuis le mic
 *   hal_audio_play(buf, len)  — joue len samples PCM 16-bit sur le codec
 *   hal_audio_set_volume(v)   — volume 0..100
 *   hal_audio_is_muted()      — true si codec en mute
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void  hal_audio_init(void);
int   hal_audio_read(int16_t *buf, size_t samples);  // retourne nb samples lus, -1 si erreur
void  hal_audio_play(const int16_t *buf, size_t samples);
void  hal_audio_set_volume(uint8_t vol_pct);         // 0..100
bool  hal_audio_is_muted(void);
void  hal_audio_mute(bool mute);

#ifdef __cplusplus
}
#endif
