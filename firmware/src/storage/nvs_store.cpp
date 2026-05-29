// ============================================================
// CompagnonV2 — storage/nvs_store.cpp
// fix #5 : les get* ouvraient en readOnly=true ce qui génère
//          ESP_ERR_NVS_NOT_FOUND (code 0x1102) et remonte en
//          Preferences comme "NOT_FOUND" sur flash vierge.
//          Solution : ouvrir en readOnly=false → le namespace
//          est créé automatiquement s'il n'existe pas, et la
//          valeur par défaut est retournée sans erreur.
//          Les set* ne changent pas (déjà readOnly=false).
// ============================================================
#include "nvs_store.h"
#include <Preferences.h>

namespace NvsStore {

static Preferences _prefs;

String getString(const char* ns, const char* key, const String& def) {
    // fix #5 : readOnly=false pour créer le namespace si absent
    if (!_prefs.begin(ns, false)) return def;
    String v = _prefs.getString(key, def);
    _prefs.end();
    return v;
}

int32_t getInt(const char* ns, const char* key, int32_t def) {
    if (!_prefs.begin(ns, false)) return def;
    int32_t v = _prefs.getInt(key, def);
    _prefs.end();
    return v;
}

bool getBool(const char* ns, const char* key, bool def) {
    if (!_prefs.begin(ns, false)) return def;
    bool v = _prefs.getBool(key, def);
    _prefs.end();
    return v;
}

bool setString(const char* ns, const char* key, const String& val) {
    if (!_prefs.begin(ns, false)) return false;
    bool ok = _prefs.putString(key, val) > 0;
    _prefs.end();
    return ok;
}

bool setInt(const char* ns, const char* key, int32_t val) {
    if (!_prefs.begin(ns, false)) return false;
    bool ok = _prefs.putInt(key, val) > 0;
    _prefs.end();
    return ok;
}

bool setBool(const char* ns, const char* key, bool val) {
    if (!_prefs.begin(ns, false)) return false;
    bool ok = _prefs.putBool(key, val);
    _prefs.end();
    return ok;
}

bool remove(const char* ns, const char* key) {
    if (!_prefs.begin(ns, false)) return false;
    bool ok = _prefs.remove(key);
    _prefs.end();
    return ok;
}

bool clear(const char* ns) {
    if (!_prefs.begin(ns, false)) return false;
    bool ok = _prefs.clear();
    _prefs.end();
    return ok;
}

} // namespace NvsStore
