#pragma once
/*
 * secrets_template.h — Template de développement UNIQUEMENT
 *
 * CE FICHIER EST POUR LE DEV LOCAL UNIQUEMENT.
 * En production, toutes les valeurs ci-dessous sont
 * configurées depuis la PWA CompagnonV2 via BLE → NVS.
 *
 * Usage :
 *   1. Copier ce fichier vers secrets.h
 *   2. Remplir les valeurs pour vos tests
 *   3. Ne jamais commiter secrets.h (ajouté au .gitignore)
 *
 * Paramètres configurables depuis la PWA :
 *   - BLE_DEVICE_NAME  → NVS key "ble_name"
 *   - WAKE_WORD        → NVS key "wake_word"
 *   - WiFi SSID/PWD    → NVS via commande BLE wifi_provision
 *   - Toutes les API keys → NVS via commande BLE set_api_key
 */

// Ne décommenter que pour le dev local :
// #define DEV_BLE_NAME        "CompagnonV2"
// #define DEV_WAKE_WORD       "nestor"
// #define DEV_WIFI_SSID       "MonReseau"
// #define DEV_WIFI_PASSWORD   "MonMotDePasse"
// #define DEV_GROQ_API_KEY    "gsk_..."
// #define DEV_GEMINI_API_KEY  "AIza..."
