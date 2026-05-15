// ============================================================
// CompagnonV2 — ui/launcher.h
// Carousel LVGL 8 — 5 apps
// Navigation : BTN_BOOT (short=suivant), BTN_USER (short=suivant, long=ouvrir)
// Touch swipe gauche/droite
// ============================================================
#pragma once

namespace ui {

void launcher_init();
void launcher_show();          // retour au launcher depuis une app
void launcher_btn_next();      // bouton suivant
void launcher_btn_open();      // ouvrir app sélectionnée
void launcher_touch_swipe_left();
void launcher_touch_swipe_right();

} // namespace ui
