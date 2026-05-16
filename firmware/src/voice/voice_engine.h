#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

void voice_engine_init();
void voice_engine_set_silent(bool silent);
bool voice_engine_is_listening();

// Synthèse vocale (TTS Groq) — respecte le mode silencieux
void voice_engine_speak(const char* text);

#ifdef __cplusplus
}
#endif
