// ============================================================
// CompagnonV2 — storage/nvs_store.h
// Wrapper centralisé NVS (Preferences). Aucun accès direct
// à Preferences en dehors de ce module.
// ============================================================
#pragma once
#include <Arduino.h>

namespace NvsStore {

// Lecture
String  getString(const char* ns, const char* key, const String& def = "");
int32_t getInt   (const char* ns, const char* key, int32_t def = 0);
bool    getBool  (const char* ns, const char* key, bool def = false);

// Écriture
bool setString(const char* ns, const char* key, const String& val);
bool setInt   (const char* ns, const char* key, int32_t val);
bool setBool  (const char* ns, const char* key, bool val);

// Suppression
bool remove(const char* ns, const char* key);
bool clear (const char* ns);

} // namespace NvsStore
