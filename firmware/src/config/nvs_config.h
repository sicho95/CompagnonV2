#pragma once
#include <stdbool.h>

#define NVS_KEY_GROQ        "groq_key"
#define NVS_KEY_GEMINI      "gemini_key"
#define NVS_KEY_SERPER      "serper_key"
#define NVS_KEY_OPENROUTER  "openrouter_k"
#define NVS_KEY_TWELVEDATA  "twelvedata_k"
#define NVS_KEY_METEO       "meteo_key"
#define NVS_KEY_SPOTIFY_ID  "spotify_id"
#define NVS_KEY_SPOTIFY_SEC "spotify_sec"
#define NVS_KEY_TUYA_ID     "tuya_id"
#define NVS_KEY_TUYA_SEC    "tuya_sec"
#define NVS_KEY_ECOVACS_U   "ecovacs_user"
#define NVS_KEY_ECOVACS_P   "ecovacs_pass"

#ifdef __cplusplus
extern "C" {
#endif

void nvs_config_init();
bool nvs_set_api_key(const char* key, const char* val);
bool nvs_get_api_key(const char* key, char* out, size_t len);
void nvs_clear_api_key(const char* key);
void nvs_list_api_keys_json(char* buf, size_t len);
bool nvs_set_bool(const char* key, bool val);
bool nvs_get_bool(const char* key, bool def);

#ifdef __cplusplus
}
#endif
