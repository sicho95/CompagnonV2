// ============================================================
// CompagnonV2 — include/secrets.h
// Credentials de développement — NE PAS COMMITTER en production
//
// En production : ces valeurs sont poussées dans le NVS
// via la PWA (BLE characteristic write) et ce fichier
// est ignoré (.gitignore : include/secrets.h)
// ============================================================
#pragma once

// ── WiFi ─────────────────────────────────────────────────
#define SECRET_WIFI_SSID     "MonSSID"
#define SECRET_WIFI_PASS     "MonMotDePasse"

// ── Groq API ─────────────────────────────────────────────
#define SECRET_GROQ_API_KEY  "gsk_xxxxxxxxxxxxxxxxxxxx"

// ── BLE AP (optionnel, défaut si vide) ─────────────────────
#define SECRET_AP_SSID       "Compagnon-AP"
#define SECRET_AP_PASS       "compagnon2024"
