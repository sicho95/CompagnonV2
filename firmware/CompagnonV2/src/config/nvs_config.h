#pragma once
/*
 * config/nvs_config.h — Gestion NVS CompagnonV2
 *
 * NOMS RESERVES IDF (ne pas utiliser comme noms de fonctions) :
 *   nvs_set_u8, nvs_get_u8, nvs_set_str, nvs_get_str
 *   -> renommes en cfg_set_u8, cfg_get_u8, cfg_set_str, cfg_get_str
 */
#include <Arduino.h>
#include <stdbool.h>

// ── Clés API ────────────────────────────────────────────────────────────────────────
#define NVS_KEY_GROQ        "groq_key"
#define NVS_KEY_GEMINI      "gemini_key"
#define NVS_KEY_SERPER      "serper_key"
#define NVS_KEY_OPENROUTER  "openrtr_key"
#define NVS_KEY_TWELVEDATA  "twdata_key"
#define NVS_KEY_METEO       "meteo_key"   // token Météo-Concept (api.meteo-concept.com)
#define NVS_KEY_SPOTIFY_ID  "spotify_id"
#define NVS_KEY_SPOTIFY_SEC "spotify_sec"
#define NVS_KEY_TUYA_ID     "tuya_id"
#define NVS_KEY_TUYA_SEC    "tuya_sec"
#define NVS_KEY_TUYA_REGION "tuya_region"
#define NVS_KEY_TUYA_USER   "tuya_user"
#define NVS_KEY_ECOVACS_U   "ecovacs_u"
#define NVS_KEY_ECOVACS_P   "ecovacs_p"
#define NVS_KEY_ECOVACS_CC  "ecovacs_cc"
#define NVS_KEY_ECOVACS_DEV "ecovacs_dev"

// ── Paramètres OS ──────────────────────────────────────────────────────────────────
#define NVS_KEY_BLE_NAME    "ble_name"
#define NVS_KEY_WAKE_WORD   "wake_word"
#define NVS_KEY_SILENT      "silent_mode"
#define NVS_KEY_VOLUME      "volume"
#define NVS_KEY_LAUNCHER_BG "launcher_bg"

void    nvs_config_init(void);

bool    nvs_set_api_key(const char *key, const char *value);
bool    nvs_get_api_key(const char *key, char *out, size_t out_len);
void    nvs_clear_api_key(const char *key);
void    nvs_list_api_keys_json(char *out, size_t out_len);

bool    nvs_set_bool(const char *key, bool value);
bool    nvs_get_bool(const char *key, bool default_val);

// Renommes pour eviter conflit avec IDF nvs.h
bool    cfg_set_u8(const char *key, uint8_t value);
uint8_t cfg_get_u8(const char *key, uint8_t default_val);
bool    cfg_set_str(const char *key, const char *value);
bool    cfg_get_str(const char *key, char *out, size_t out_len, const char *default_val);
