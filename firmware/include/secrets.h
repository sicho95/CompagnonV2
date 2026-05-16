// ============================================================
// CompagnonV2 — include/secrets.h
// Credentials de développement — NE PAS COMMITTER en production
//
// En production : toutes ces valeurs sont poussées dans le NVS
// via la PWA (BLE characteristic write).
// Le provisioning au boot est conditionnel (écrit NVS seulement
// si la clé est encore vide) — la PWA peut donc écraser sans
// que le firmware réinitialise au redémarrage.
// ============================================================
#pragma once

// ─────────────────────────────────────────────────────────────
// WiFi
// ─────────────────────────────────────────────────────────────
#define SECRET_WIFI_SSID        "MonSSID"
#define SECRET_WIFI_PASS        "MonMotDePasse"

// Mode AP (fallback si connexion échoue)
#define SECRET_AP_SSID          "Compagnon-AP"
#define SECRET_AP_PASS          "compagnon2024"

// ─────────────────────────────────────────────────────────────
// Groq  (STT Whisper + Chat LLaMA + TTS PlayAI)
// https://console.groq.com/keys
// NVS namespace "app", clé "groq_api_key"
// ─────────────────────────────────────────────────────────────
#define SECRET_GROQ_API_KEY     "gsk_xxxxxxxxxxxxxxxxxxxx"

// ─────────────────────────────────────────────────────────────
// Twelve Data  (app_bourse — cours actions/ETF/crypto)
// https://twelvedata.com/account/api-keys
// NVS namespace "bourse", clé "td_api_key"
// ─────────────────────────────────────────────────────────────
#define SECRET_TWELVE_DATA_KEY  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

// ─────────────────────────────────────────────────────────────
// WeatherAPI  (app_meteo — météo + prévisions)
// https://www.weatherapi.com/my/
// NVS namespace "meteo", clé "weather_api_key"
// ─────────────────────────────────────────────────────────────
#define SECRET_WEATHER_API_KEY  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

// ─────────────────────────────────────────────────────────────
// Tuya  (objets connectés)
// https://iot.tuya.com/ → Cloud → Projets → Access ID / Secret
// NVS namespace "tuya"
// ─────────────────────────────────────────────────────────────
#define SECRET_TUYA_ACCESS_ID   "xxxxxxxxxxxxxxxxxxxx"
#define SECRET_TUYA_ACCESS_KEY  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
// Région : "eu" | "us" | "cn" | "in"
#define SECRET_TUYA_REGION      "eu"

// ─────────────────────────────────────────────────────────────
// Ecovacs  (aspirateur X8 Pro Omni)
// Compte Ecovacs Home / EcovacsAPI
// NVS namespace "ecovacs"
// ─────────────────────────────────────────────────────────────
#define SECRET_ECOVACS_ACCOUNT  "mon.email@exemple.com"
#define SECRET_ECOVACS_PASSWORD "MonMotDePasse"
// Continent : "eu" | "na" | "as"
#define SECRET_ECOVACS_CONTINENT "eu"
