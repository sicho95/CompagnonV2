// ============================================================
// CompagnonV2 — hal/audio.h
// Shim d'alias vers hal::audio_init (hal_audio.h / hal_audio.cpp)
// ============================================================
#pragma once
#include "hal_audio.h"

// Alias flat C pour le .ino
inline bool hal_audio_init() { return hal::audio_init(); }
