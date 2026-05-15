#pragma once
// ============================================================
// CompagnonV2 — secrets_template.h
// Copier ce fichier en secrets.h et remplir les valeurs.
// NE PAS versionner secrets.h (ajouté dans .gitignore)
// ============================================================

// ─── WiFi de développement (fallback si provisioning BLE non fait) ─
#define WIFI_DEV_SSID       "YOUR_SSID"
#define WIFI_DEV_PASSWORD   "YOUR_PASSWORD"

// ─── Wake word ──────────────────────────────────────
#define WAKE_WORD           "Nestor"

// ─── APIs ──────────────────────────────────────────
#define GROQ_API_KEY        "YOUR_GROQ_API_KEY"
#define OPENWEATHER_API_KEY "YOUR_OPENWEATHER_KEY"
#define ALPHA_VANTAGE_KEY   "YOUR_ALPHAVANTAGE_KEY"

// ─── BLE Device name ──────────────────────────────
#define BLE_DEVICE_NAME     "Compagnon"
