// ============================================================
// CompagnonV2 — ui/status_bar.cpp
// Barre de statut fixe en haut, rendue sur lv_layer_top()
// Hauteur : STATUS_BAR_H pixels, fond semi-transparent
// Affiche le titre "CompagnonV2" centre
// ============================================================
#include "status_bar.h"
#include <lvgl.h>
#include <Arduino.h>

#define STATUS_BAR_H  28

static lv_obj_t* _bar   = nullptr;
static lv_obj_t* _label = nullptr;

void ui_status_bar_init() {
    if (lv_display_get_default() == nullptr) {
        Serial.println("[UI] status_bar init SKIP — aucun display LVGL");
        return;
    }

    _bar = lv_obj_create(lv_layer_top());
    lv_obj_set_size(_bar, LV_HOR_RES, STATUS_BAR_H);
    lv_obj_set_pos(_bar, 0, 0);
    lv_obj_set_style_bg_color(_bar, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_bg_opa(_bar, LV_OPA_90, 0);
    lv_obj_set_style_border_width(_bar, 0, 0);
    lv_obj_set_style_pad_all(_bar, 0, 0);
    lv_obj_set_style_radius(_bar, 0, 0);
    lv_obj_clear_flag(_bar, LV_OBJ_FLAG_SCROLLABLE);
    // LVGL 9 : lv_obj_set_style_z_index n'existe pas — utiliser move_foreground()
    lv_obj_move_foreground(_bar);

    _label = lv_label_create(_bar);
    lv_label_set_text(_label, "CompagnonV2");
    lv_obj_set_style_text_font(_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(_label, lv_color_hex(0x7EB8D4), 0);
    lv_obj_align(_label, LV_ALIGN_CENTER, 0, 0);

    Serial.println("[UI] status_bar init OK");
}

void ui_status_bar_tick() {
    // Placeholder — peut afficher l'heure RTC ici
}

void ui_power_menu_show() {
    Serial.println("[UI] power menu");
}
