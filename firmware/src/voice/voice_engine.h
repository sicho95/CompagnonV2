#pragma once
/*
 * voice/voice_engine.h — Moteur vocal CompagnonV2
 *
 * Responsabilités :
 *   - Wake word continu (ESP-SR WakeNet) sur Core 0
 *   - Réveil depuis light sleep sur détection wake word
 *   - Capture STT (Groq Whisper via HTTP ou relais BLE si pas de WiFi)
 *   - Dispatch du texte reconnu vers l'orchestrateur
 *   - TTS (Groq PlayAI ou Web Speech via BLE relay)
 *   - Bouton micro LVGL : déclenche le même pipeline que le wake word
 *   - Mode silencieux : pas de son TTS ni de bip (mais wake word reste actif)
 *
 * Workflow :
 *   hal_audio_init() doit être appelé avant voice_engine_init()
 */

#ifdef __cplusplus
extern "C" {
#endif

void  voice_engine_init(void);            // crée la tâche FreeRTOS Core 0
void  voice_engine_deinit(void);

// Déclenchement manuel (bouton micro LVGL ou depuis une app)
void  voice_engine_start_listen(void);    // capture STT immédiate sans wake word
void  voice_engine_speak(const char *text); // TTS du texte fourni

// Configuration
void  voice_engine_set_silent(bool silent); // mode silencieux OS
bool  voice_engine_is_silent(void);
void  voice_engine_set_wake_word(const char *word); // configuré depuis PWA via NVS

// Callback : appelé quand un texte est reconnu (wake word OU bouton micro)
// Signature : void callback(const char *text, const char *source_app)
typedef void (*voice_dispatch_cb_t)(const char *text, const char *source_app);
void  voice_engine_set_dispatch_cb(voice_dispatch_cb_t cb);

#ifdef __cplusplus
}
#endif
