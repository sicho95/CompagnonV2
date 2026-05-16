// ============================================================
// CompagnonV2 — hal/audio.h
// Shim d'alias vers hal_audio (ES7210 + ES8311)
// L'implémentation réelle est dans hal_audio.h / hal_audio.cpp
// ============================================================
#pragma once
#include "hal_audio.h"

// Alias flat C pour le .ino
inline bool hal_audio_init() { return audio_init(); }
