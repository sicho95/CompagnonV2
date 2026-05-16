#include "nvs_config.h"
#include <Preferences.h>
#include <Arduino.h>
#include <cstring>
#include <cstdio>

static Preferences _prefs;

void nvs_config_init() {
    _prefs.begin("compagnon", false);
    Serial.println("[NVS] init OK");
}

bool nvs_set_api_key(const char* key, const char* val) {
    return _prefs.putString(key, val) > 0;
}
bool nvs_get_api_key(const char* key, char* out, size_t len) {
    String s = _prefs.getString(key, "");
    if (s.length() == 0) return false;
    strncpy(out, s.c_str(), len - 1); out[len-1] = 0;
    return true;
}
void nvs_clear_api_key(const char* key) { _prefs.remove(key); }

void nvs_list_api_keys_json(char* buf, size_t len) {
    static const char* keys[] = {
        NVS_KEY_GROQ, NVS_KEY_GEMINI, NVS_KEY_SERPER,
        NVS_KEY_OPENROUTER, NVS_KEY_TWELVEDATA, NVS_KEY_METEO,
        NVS_KEY_SPOTIFY_ID, NVS_KEY_SPOTIFY_SEC,
        NVS_KEY_TUYA_ID, NVS_KEY_TUYA_SEC,
        NVS_KEY_ECOVACS_U, NVS_KEY_ECOVACS_P, nullptr
    };
    size_t pos = 0;
    pos += snprintf(buf+pos, len-pos, "{");
    for (int i = 0; keys[i]; i++) {
        bool set = _prefs.getString(keys[i], "").length() > 0;
        pos += snprintf(buf+pos, len-pos, "\"%s\":%s%s",
                        keys[i], set?"true":"false", keys[i+1]?",":"");
    }
    snprintf(buf+pos, len-pos, "}");
}

bool nvs_set_bool(const char* key, bool val) { return _prefs.putBool(key, val); }
bool nvs_get_bool(const char* key, bool def)  { return _prefs.getBool(key, def); }
