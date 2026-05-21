#include "launcher.h"
#include <lvgl.h>
#include <Arduino.h>

// ── Launcher minimal de test ─────────────────────────────────────────────────
// Crée un écran blanc avec un label centré pour prouver que LVGL flush correctement.
// Remplacer par la vraie grille d'icônes une fois l'affichage validé.

static lv_obj_t* _screen = nullptr;

void ui_launcher_init() {
    // Écran dédié launcher
    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x1A1A2E), 0); // fond bleu nuit
    lv_obj_set_style_bg_opa(_screen,  LV_OPA_COVER, 0);

    // Label centré
    lv_obj_t* label = lv_label_create(_screen);
    lv_label_set_text(label, "CompagnonV2\nLVGL OK");
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    lv_obj_center(label);

    // Barre du bas (accent cyan)
    lv_obj_t* bar = lv_obj_create(_screen);
    lv_obj_set_size(bar, LV_HOR_RES, 6);
    lv_obj_set_pos(bar, 0, LV_VER_RES - 6);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x00D4FF), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);

    // Charger l'écran
    lv_scr_load(_screen);

    Serial.println("[UI] launcher init OK");
}

void ui_launcher_btn_tick() {}
