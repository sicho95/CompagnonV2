// ============================================================
// CompagnonV2 — ui/status_bar.cpp
// fix #2 : barre créée directement sur lv_scr_act() et
//          promue au premier plan APRÈS que le launcher ait
//          appelé lv_scr_load(). On expose aussi
//          ui_status_bar_raise() à appeler depuis launcher.cpp.
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

    // fix #2 : lv_layer_top() en LVGL9 n'est pas opaque par défaut et
    // n'est pas rendu au-dessus d'un nouveau scr chargé via lv_scr_load().
    // On crée la barre sur lv_layer_top() mais on reporte le move_foreground
    // APRES lv_scr_load() via ui_status_bar_raise().
    _bar = lv_obj_create(lv_layer_top());
    lv_obj_set_size(_bar, LV_HOR_RES, STATUS_BAR_H);
    lv_obj_set_pos(_bar, 0, 0);
    lv_obj_set_style_bg_color(_bar, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_bg_opa(_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_bar, 1, 0);
    lv_obj_set_style_border_side(_bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(_bar, lv_color_hex(0x1A2A3A), 0);
    lv_obj_set_style_pad_all(_bar, 0, 0);
    lv_obj_set_style_radius(_bar, 0, 0);
    lv_obj_clear_flag(_bar, LV_OBJ_FLAG_SCROLLABLE);

    _label = lv_label_create(_bar);
    lv_label_set_text(_label, "CompagnonV2");
    lv_obj_set_style_text_font(_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(_label, lv_color_hex(0x7EB8D4), 0);
    lv_obj_align(_label, LV_ALIGN_CENTER, 0, 0);

    Serial.println("[UI] status_bar init OK");
}

// fix #2 : appelé par ui_launcher_init() APRÈS lv_scr_load()
// pour forcer la barre au premier plan sur le nouveau screen.
void ui_status_bar_raise() {
    if (_bar) lv_obj_move_foreground(_bar);
}

void ui_status_bar_tick() {
    // Placeholder — afficher l'heure RTC ici
}

void ui_power_menu_show() {
    Serial.println("[UI] power menu");
}
