#include "launcher.h"
#include "../hal/display.h"
#include <lvgl.h>
#include <Arduino.h>

// ── Launcher minimal de test ──────────────────────────────────────────────────
// Fond bleu nuit + label centré + barre cyan = preuve que LVGL flush.
// lv_font_montserrat_14 : activée par défaut dans lv_conf.h (LV_FONT_MONTSERRAT_14 1)
// Remplacer par la vraie grille d'icônes une fois l'affichage validé.

static lv_obj_t* _screen = nullptr;

void ui_launcher_init() {
    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, 0);

    // Label centré — fonte montserrat_14 activée par défaut dans lv_conf.h
    lv_obj_t* label = lv_label_create(_screen);
    lv_label_set_text(label, "CompagnonV2\nLVGL OK");
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0); // fonte sûre
    lv_obj_center(label);

    // Barre cyan en bas
    lv_obj_t* bar = lv_obj_create(_screen);
    lv_obj_set_size(bar, LV_HOR_RES, 8);
    lv_obj_set_pos(bar, 0, LV_VER_RES - 8);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x00D4FF), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);

    // Charger l'écran et forcer un flush immédiat
    lv_scr_load(_screen);
    hal::display_force_refresh(); // lv_refr_now() — ne pas attendre loop()

    Serial.println("[UI] launcher init OK");
}

void ui_launcher_btn_tick() {}
