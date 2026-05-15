/**
 * secrets_template.h — DEV UNIQUEMENT
 * Copier en secrets.h (non commité, dans .gitignore)
 *
 * EN PRODUCTION : tous les paramètres (WiFi, BLE name, wake word,
 * clés API) sont configurés UNIQUEMENT depuis la PWA et poussés
 * via BLE → NVS. Ce fichier ne sert qu'au flash initial de dev.
 */
#pragma once

// SSID optionnel pour dev (laisser vide en prod)
#define DEV_WIFI_SSID ""
#define DEV_WIFI_PASS ""
