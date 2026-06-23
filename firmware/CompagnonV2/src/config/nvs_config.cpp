// ============================================================
// CompagnonV2 — config/nvs_config.cpp
// fix: nvs_flash.h inclus ici seulement (pas dans le .h)
//      noms cfg_set_u8 etc. pour eviter conflit avec IDF nvs.h
// ============================================================
#include "nvs_config.h"
#include <Preferences.h>
#include <Arduino.h>
#include <nvs_flash.h>   // ICI seulement, pas dans le .h
#include <cstring>
#include <cstdio>

static Preferences* _prefs = nullptr;

void nvs_config_init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        Serial.println("[NVS] Partition corrompue — erase + reinit");
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        Serial.printf("[NVS] nvs_flash_init FAILED: %d\n", err);
        return;
    }
    _prefs = new Preferences();
    _prefs->begin("compagnon", false);
    Serial.println("[NVS] init OK");
}

bool nvs_set_api_key(const char* key, const char* val) {
    if (!_prefs) return false;
    return _prefs->putString(key, val) > 0;
}
bool nvs_get_api_key(const char* key, char* out, size_t len) {
    if (!_prefs) return false;
    String s = _prefs->getString(key, "");
    if (s.length() == 0) return false;
    strncpy(out, s.c_str(), len - 1);
    out[len - 1] = 0;
    return true;
}
void nvs_clear_api_key(const char* key) { if (_prefs) _prefs->remove(key); }

void nvs_list_api_keys_json(char* buf, size_t len) {
    static const char* keys[] = {
        NVS_KEY_GROQ, NVS_KEY_GEMINI, NVS_KEY_SERPER,
        NVS_KEY_OPENROUTER, NVS_KEY_TWELVEDATA, NVS_KEY_METEO,
        NVS_KEY_SPOTIFY_ID, NVS_KEY_SPOTIFY_SEC,
        NVS_KEY_TUYA_ID, NVS_KEY_TUYA_SEC, NVS_KEY_TUYA_REGION, NVS_KEY_TUYA_USER,
        NVS_KEY_ECOVACS_U, NVS_KEY_ECOVACS_P, NVS_KEY_ECOVACS_CC, NVS_KEY_ECOVACS_DEV,
        nullptr
    };
    if (!_prefs) { snprintf(buf, len, "{}"); return; }
    size_t pos = 0;
    pos += snprintf(buf + pos, len - pos, "{");
    for (int i = 0; keys[i]; i++) {
        bool set = _prefs->getString(keys[i], "").length() > 0;
        pos += snprintf(buf + pos, len - pos, "\"%s\":%s%s",
            keys[i], set ? "true" : "false", keys[i+1] ? "," : "");
    }
    snprintf(buf + pos, len - pos, "}");
}

bool nvs_set_bool(const char* key, bool val)  { return _prefs ? _prefs->putBool(key, val)  : false; }
bool nvs_get_bool(const char* key, bool def)  { return _prefs ? _prefs->getBool(key, def)  : def;   }
bool cfg_set_u8(const char* key, uint8_t val) { return _prefs ? _prefs->putUChar(key, val) > 0 : false; }
uint8_t cfg_get_u8(const char* key, uint8_t def) { return _prefs ? _prefs->getUChar(key, def) : def; }
bool cfg_set_str(const char* key, const char* val) { return _prefs ? _prefs->putString(key, val) > 0 : false; }
bool cfg_get_str(const char* key, char* out, size_t len, const char* def) {
    if (!_prefs) { if (def && out && len) { strncpy(out, def, len-1); out[len-1]=0; } return false; }
    String s = _prefs->getString(key, def ? def : "");
    strncpy(out, s.c_str(), len - 1);
    out[len - 1] = 0;
    return s.length() > 0;
}
