// ============================================================
// CompagnonV2 — ui/launcher.cpp
// Grille 3x2 d'icones, tap → os::app_launch()
// Fix : icones ASCII (pas d'emoji — evite les rectangles),
//       labels UTF-8 corrects, grille centree, touch LVGL actif
// ============================================================
#include "launcher.h"
#include "../hal/display.h"
#include "../system/os_kernel.h"
#include <lvgl.h>
#include <Arduino.h>

#define STATUS_BAR_H  28

// ── Descripteurs des tuiles ──────────────────────────────────
struct TileDesc {
    os::AppId   id;
    const char* icon;   // symbole ASCII/Latin simple (lv_font_montserrat_36 ne contient pas les emoji)
    const char* label;
};

static const TileDesc TILES[] = {
    { os::AppId::NESTOR,  "N",  "Nestor"  },
    { os::AppId::METEO,   "M",  "M\xc3\xa9t\xc3\xa9o"   },   // Météo
    { os::AppId::BOURSE,  "B",  "Bourse"  },
    { os::AppId::RADARS,  "R",  "Radars"  },
    { os::AppId::RAPPELS, "!",  "Rappels" },
    { os::AppId::NONE,    "S",  "R\xc3\xa9glages" },            // Réglages
};
static constexpr int N_TILES = 6;

// ── State ────────────────────────────────────────────────────
static lv_obj_t* _screen = nullptr;
static lv_obj_t* _grid   = nullptr;

// ── Callback tap tuile ───────────────────────────────────────
static void _tile_tap_cb(lv_event_t* e) {
    auto* desc = (TileDesc*)lv_event_get_user_data(e);
    if (!desc) return;
    if (desc->id != os::AppId::NONE) {
        Serial.printf("[UI] launch app %d\n", (int)desc->id);
        os::app_launch(desc->id);
    } else {
        Serial.println("[UI] Reglages (TODO)");
    }
}

// ── API publique ─────────────────────────────────────────────
void ui_launcher_init() {
    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(_screen, LV_OBJ_FLAG_SCROLLABLE);

    // Zone utile sous la status bar
    int32_t grid_y    = STATUS_BAR_H + 8;
    int32_t grid_h    = LV_VER_RES - grid_y - 8;
    int32_t grid_w    = LV_HOR_RES - 16;  // marges gauche/droite de 8px

    // Grille 3 colonnes, 2 lignes — centrée
    _grid = lv_obj_create(_screen);
    lv_obj_set_size(_grid, grid_w, grid_h);
    lv_obj_set_pos(_grid, 8, grid_y);
    lv_obj_set_style_bg_opa(_grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_grid, 0, 0);
    lv_obj_set_style_pad_all(_grid, 4, 0);
    lv_obj_set_style_pad_gap(_grid, 8, 0);
    lv_obj_clear_flag(_grid, LV_OBJ_FLAG_SCROLLABLE);

    static lv_coord_t col_dsc[] = {
        LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
        LV_GRID_TEMPLATE_LAST
    };
    static lv_coord_t row_dsc[] = {
        LV_GRID_FR(1), LV_GRID_FR(1),
        LV_GRID_TEMPLATE_LAST
    };
    lv_obj_set_grid_dsc_array(_grid, col_dsc, row_dsc);
    lv_obj_set_layout(_grid, LV_LAYOUT_GRID);

    for (int i = 0; i < N_TILES; i++) {
        lv_obj_t* tile = lv_obj_create(_grid);
        lv_obj_set_size(tile, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_grid_cell(tile,
            LV_GRID_ALIGN_STRETCH, i % 3, 1,
            LV_GRID_ALIGN_STRETCH, i / 3, 1);

        lv_obj_set_style_bg_color(tile, lv_color_hex(0x1C1C2E), 0);
        lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(tile, 16, 0);
        lv_obj_set_style_border_width(tile, 1, 0);
        lv_obj_set_style_border_color(tile, lv_color_hex(0x2A2A40), 0);
        lv_obj_set_style_pad_all(tile, 0, 0);
        lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
        // Etat presse
        lv_obj_set_style_bg_color(tile, lv_color_hex(0x2E2E50), LV_STATE_PRESSED);
        lv_obj_add_flag(tile, LV_OBJ_FLAG_CLICKABLE);

        // Icone (grande lettre centrée)
        lv_obj_t* icon_lbl = lv_label_create(tile);
        lv_label_set_text(icon_lbl, TILES[i].icon);
        lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_36, 0);
        lv_obj_set_style_text_color(icon_lbl, lv_color_white(), 0);
        lv_obj_align(icon_lbl, LV_ALIGN_CENTER, 0, -12);

        // Nom de l'app
        lv_obj_t* name_lbl = lv_label_create(tile);
        lv_label_set_text(name_lbl, TILES[i].label);
        lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(name_lbl, lv_color_hex(0xCCCCDD), 0);
        lv_obj_align(name_lbl, LV_ALIGN_BOTTOM_MID, 0, -10);

        lv_obj_add_event_cb(tile, _tile_tap_cb, LV_EVENT_CLICKED,
                            (void*)&TILES[i]);
    }

    lv_scr_load(_screen);
    hal::display_force_refresh();
    Serial.println("[UI] launcher OK — grille 3x2 avec touch LVGL");
}

void ui_launcher_show() {
    if (_screen) lv_scr_load_anim(_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
}

void ui_launcher_btn_tick() {}
