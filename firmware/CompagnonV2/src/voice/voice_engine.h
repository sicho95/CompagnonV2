#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

void voice_engine_init();
void voice_engine_set_silent(bool silent);
bool voice_engine_is_listening();

// fix: fonctions manquantes utilisées par ui_reminders / app_rappels
bool voice_engine_is_silent();           // retourne l'état silencieux courant
void voice_engine_start_recording();     // déclenche une capture micro manuelle
bool voice_engine_wake_word_detected();  // vrai si wake word détecté depuis dernier tick

// Synthèse vocale (TTS Groq) — respecte le mode silencieux
void voice_engine_speak(const char* text);

#ifdef __cplusplus
}
#endif
