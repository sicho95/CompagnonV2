#pragma once
/*
 * config/nvs_config.h — Gestion NVS CompagnonV2
 *
 * Fix 1 : #include <Arduino.h> en tête pour size_t + String
 *
 * Namespace : "compagnon"
 * Toutes les clés NVS sont <= 15 caractères (limite ESP-IDF).
 */
#include <Arduino.h>   // size_t, String, uint8_t — Fix 1
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ── Clés API ──────────────────────────────────────────────────────────────
#define NVS_KEY_GROQ        "groq_key"
#define NVS_KEY_GEMINI      "gemini_key"
#define NVS_KEY_SERPER      "serper_key"
#define NVS_KEY_OPENROUTER  "openrtr_key"
#define NVS_KEY_TWELVEDATA  "twdata_key"
#define NVS_KEY_METEO       "meteo_key"
#define NVS_KEY_SPOTIFY_ID  "spotify_id"
#define NVS_KEY_SPOTIFY_SEC "spotify_sec"
#define NVS_KEY_TUYA_ID     "tuya_id"
#define NVS_KEY_TUYA_SEC    "tuya_sec"
#define NVS_KEY_ECOVACS_U   "ecovacs_u"
#define NVS_KEY_ECOVACS_P   "ecovacs_p"

// ── Paramètres OS (configurables depuis PWA) ──────────────────────────────
#define NVS_KEY_BLE_NAME    "ble_name"
#define NVS_KEY_WAKE_WORD   "wake_word"
#define NVS_KEY_SILENT      "silent_mode"
#define NVS_KEY_VOLUME      "volume"

void    nvs_config_init(void);

bool    nvs_set_api_key(const char *key, const char *value);
bool    nvs_get_api_key(const char *key, char *out, size_t out_len);
void    nvs_clear_api_key(const char *key);
void    nvs_list_api_keys_json(char *out, size_t out_len);

bool    nvs_set_bool(const char *key, bool value);
bool    nvs_get_bool(const char *key, bool default_val);
bool    nvs_set_u8(const char *key, uint8_t value);
uint8_t nvs_get_u8(const char *key, uint8_t default_val);
bool    nvs_set_str(const char *key, const char *value);
bool    nvs_get_str(const char *key, char *out, size_t out_len, const char *default_val);

#ifdef __cplusplus
}
#endif
