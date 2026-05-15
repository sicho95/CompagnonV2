// ============================================================
// CompagnonV2 — ui/status_bar.h
// Barre de statut 480×36 LVGL 8
// Heure | BLE | WiFi | Batterie%
// ============================================================
#pragma once

namespace ui {

void status_bar_init();
void status_bar_tick(); // appelé chaque seconde par task_os_main

} // namespace ui
