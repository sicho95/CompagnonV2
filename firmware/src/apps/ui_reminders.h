#pragma once
// ============================================================
// CompagnonV2 — App Rappels UI (LVGL 8.x)
// Popup liste → tap → done
// Formulaire nouveau rappel (touch input ou voice)
// Bouton mic intégré → voice::start_recording()
// Sync BLE → PWA via AGENT_SYNC
// ============================================================
#include <lvgl.h>

namespace apps {
namespace reminders {

void init();    // 1x au boot
void start();   // crée l'écran LVGL
void stop();    // détruit l'écran LVGL, libère RAM
void tick();    // appelé chaque 100 ms par task_os_main

// Intent vocal : "rappelle-moi de X le Y à Z"
void handle_voice_intent(const char* text);

// Sync BLE bidirectionnelle
void        sync_from_pwa(const char* json_array);
const char* get_json_list();

} // reminders
} // apps
