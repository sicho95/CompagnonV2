#pragma once
/*
 * ui/status_bar.h — Barre de statut LVGL CompagnonV2
 *
 * Affiche (sur lv_layer_top, hauteur 36px) :
 *   - Heure et date (NTP, timezone Europe/Paris)
 *   - Icône Bluetooth (affiché si BLE device associé)
 *   - Icône WiFi (affiché si connecté + niveau signal)
 *   - Jauge batterie colorée + pourcentage :
 *       vert   si ≥ 50%
 *       orange si 20..49%
 *       rouge  si < 20%
 *     + éclair ⚡ si en charge
 *   - Icône micro (animée quand wake word actif / STT en cours)
 */

#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

void ui_status_bar_init(void);
void ui_status_bar_tick(void);  // à appeler dans loop()

// Mises à jour depuis les modules OS
void ui_status_bar_set_ble(bool connected);
void ui_status_bar_set_wifi(bool connected, int rssi);
void ui_status_bar_set_battery(int pct, bool charging);
void ui_status_bar_set_listening(bool active);  // animation mic

#ifdef __cplusplus
}
#endif
