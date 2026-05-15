#pragma once
// ============================================================
// CompagnonV2 — device_config_template.h
// Config embarquée MINIMALE — ne contient PAS les clés API.
//
// Les clés API (Groq, OpenWeatherMap, AlphaVantage, etc.) sont
// saisies dans la PWA Compagnon et poussées de manière sécurisée
// vers l'ESP32 via BLE (caractéristique DEVICE_CONFIG).
// Elles sont ensuite stockées chiffrées en NVS.
//
// Ce fichier contient uniquement :
//   - le nom BLE du device (non secret)
//   - le wake word (non secret)
//   - un SSID/mdp WiFi de DEV (optionnel, uniquement pour le flash initial)
//     → à laisser vide en production, utiliser le provisioning PWA
// ============================================================

// ─── BLE device name (visible lors du scan BLE depuis la PWA) ────────────────
#define BLE_DEVICE_NAME     "Compagnon"

// ─── Wake word (mot clé vocal, non secret) ───────────────────────────────────
#define WAKE_WORD           "Nestor"

// ─── WiFi de développement (optionnel — laisser vide en prod) ────────────────
// En production, le WiFi se configure depuis la PWA via BLE (WIFI_PROVISION).
// Ces defines ne servent qu'au premier boot sans PWA disponible.
#define WIFI_DEV_SSID       ""   // laisser vide si provisioning BLE
#define WIFI_DEV_PASSWORD   ""   // laisser vide si provisioning BLE

// ─── NE PAS AJOUTER DE CLÉS API ICI ─────────────────────────────────────────
// Toutes les clés API sont gérées côté PWA (localStorage chiffré)
// et poussées vers l'ESP32 via BLE → stockées en NVS namespace "api_keys".
// Voir : pwa/src/device/device_settings.js + firmware/src/storage/nvs_mgr.h
