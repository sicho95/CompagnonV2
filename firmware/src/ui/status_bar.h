#pragma once
// =============================================================
// CompagnonV2 — ui/status_bar.h
// Barre de statut LVGL 9 — 440×36 px (safe area)
//
// Layout (gauche → droite) :
//   [Date/heure FR]  [icône BLE]  [icône WiFi]  [jauge batterie %]
//
// Dépendances runtime :
//   g_wifi_connected, g_ble_connected, g_battery_pct, g_charging
//   (définis comme extern volatile dans main.cpp)
// Horloge : rtc_get_time() ou time()/localtime() après NTP sync
// =============================================================
#include <lvgl.h>

/**
 * @brief Crée les widgets de la status bar sur l'écran actif.
 *        À appeler après lv_init() et lv_display_create().
 */
void status_bar_init();

/**
 * @brief Rafraîchit date/heure, icônes BLE/WiFi et jauge batterie.
 *        À appeler depuis task_os_main() toutes les 1s environ.
 */
void status_bar_tick();
