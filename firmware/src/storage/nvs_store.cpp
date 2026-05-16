// ============================================================
// CompagnonV2 — storage/nvs_store.cpp
// ============================================================
#include "nvs_store.h"
#include <Preferences.h>

namespace NvsStore {

static Preferences _prefs;

String getString(const char* ns, const char* key, const String& def) {
    _prefs.begin(ns, true);
    String v = _prefs.getString(key, def);
    _prefs.end();
    return v;
}

int32_t getInt(const char* ns, const char* key, int32_t def) {
    _prefs.begin(ns, true);
    int32_t v = _prefs.getInt(key, def);
    _prefs.end();
    return v;
}

bool getBool(const char* ns, const char* key, bool def) {
    _prefs.begin(ns, true);
    bool v = _prefs.getBool(key, def);
    _prefs.end();
    return v;
}

bool setString(const char* ns, const char* key, const String& val) {
    _prefs.begin(ns, false);
    bool ok = _prefs.putString(key, val) > 0;
    _prefs.end();
    return ok;
}

bool setInt(const char* ns, const char* key, int32_t val) {
    _prefs.begin(ns, false);
    bool ok = _prefs.putInt(key, val) > 0;
    _prefs.end();
    return ok;
}

bool setBool(const char* ns, const char* key, bool val) {
    _prefs.begin(ns, false);
    bool ok = _prefs.putBool(key, val);
    _prefs.end();
    return ok;
}

bool remove(const char* ns, const char* key) {
    _prefs.begin(ns, false);
    bool ok = _prefs.remove(key);
    _prefs.end();
    return ok;
}

bool clear(const char* ns) {
    _prefs.begin(ns, false);
    bool ok = _prefs.clear();
    _prefs.end();
    return ok;
}

} // namespace NvsStore
