// ============================================================
// CompagnonV2 — ui/launcher.cpp
// Carousel launcher : grille 3x2 d'icônes, tap → os::app_launch()
// Remplace le stub de test (fond bleu + label "LVGL OK").
// ============================================================
#include "launcher.h"
#include "../hal/display.h"
#include "../system/os_kernel.h"
#include <lvgl.h>
#include <Arduino.h>

// ── Descripteurs des tuiles ──────────────────────────────────
struct TileDesc {
    os::AppId   id;
    const char* icon;   // emoji UTF-8
    const char* label;
};

static const TileDesc TILES[] = {
    { os::AppId::NESTOR,  "\U0001F916", "Nestor"  },
    { os::AppId::METEO,   "\U000026C5", "M\xc3\xa9t\xc3\xa9o"   },
    { os::AppId::BOURSE,  "\U0001F4C8", "Bourse"  },
    { os::AppId::RADARS,  "\U0001F4E1", "Radars"  },
    { os::AppId::RAPPELS, "\u23F0",     "Rappels" },
    { os::AppId::NONE,    "\u2699\uFE0F", "R\xc3\xa9glages" },  // placeholder
};
static constexpr int N_TILES = 6;

// ── State ────────────────────────────────────────────────────
static lv_obj_t* _screen  = nullptr;
static lv_obj_t* _grid    = nullptr;

// ── Callback tap tuile ───────────────────────────────────────
static void _tile_tap_cb(lv_event_t* e) {
    auto* desc = (TileDesc*)lv_event_get_user_data(e);
    if (!desc) return;
    if (desc->id != os::AppId::NONE) {
        Serial.printf("[UI] launch app %d\n", (int)desc->id);
        os::app_launch(desc->id);
    }
}

// ── Création d'une tuile ─────────────────────────────────────
static void _make_tile(lv_obj_t* parent, const TileDesc* desc) {
    lv_obj_t* tile = lv_obj_create(parent);
    lv_obj_set_size(tile, 130, 110);
    lv_obj_set_style_bg_color(tile, lv_color_hex(0x1C1C2E), 0);
    lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(tile, 16, 0);
    lv_obj_set_style_border_width(tile, 1, 0);
    lv_obj_set_style_border_color(tile, lv_color_hex(0x2A2A40), 0);
    lv_obj_set_style_pad_all(tile, 0, 0);
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

    // Pressed state
    lv_obj_set_style_bg_color(tile, lv_color_hex(0x2E2E50), LV_STATE_PRESSED);
    lv_obj_set_style_transform_scale(tile, 240, LV_STATE_PRESSED); // 240/256 ≈ 94%
    lv_obj_add_flag(tile, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t* icon_lbl = lv_label_create(tile);
    lv_label_set_text(icon_lbl, desc->icon);
    lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(icon_lbl, lv_color_white(), 0);
    lv_obj_align(icon_lbl, LV_ALIGN_CENTER, 0, -12);

    lv_obj_t* name_lbl = lv_label_create(tile);
    lv_label_set_text(name_lbl, desc->label);
    lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(name_lbl, lv_color_hex(0xCCCCDD), 0);
    lv_obj_align(name_lbl, LV_ALIGN_BOTTOM_MID, 0, -10);

    lv_obj_add_event_cb(tile, _tile_tap_cb, LV_EVENT_CLICKED,
                        (void*)desc);
}

// ── API publique ─────────────────────────────────────────────
void ui_launcher_init() {
    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(_screen, LV_OBJ_FLAG_SCROLLABLE);

    // Titre
    lv_obj_t* title = lv_label_create(_screen);
    lv_label_set_text(title, "CompagnonV2");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x7EB8D4), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    // Grille 3 colonnes, 2 lignes
    _grid = lv_obj_create(_screen);
    lv_obj_set_size(_grid, LV_HOR_RES - 20, LV_VER_RES - 80);
    lv_obj_align(_grid, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_opa(_grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_grid, 0, 0);
    lv_obj_set_style_pad_all(_grid, 8, 0);
    lv_obj_set_style_pad_gap(_grid, 10, 0);
    lv_obj_clear_flag(_grid, LV_OBJ_FLAG_SCROLLABLE);

    static lv_coord_t col_dsc[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static lv_coord_t row_dsc[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
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
        lv_obj_set_style_bg_color(tile, lv_color_hex(0x2E2E50), LV_STATE_PRESSED);
        lv_obj_add_flag(tile, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t* icon_lbl = lv_label_create(tile);
        lv_label_set_text(icon_lbl, TILES[i].icon);
        lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_36, 0);
        lv_obj_set_style_text_color(icon_lbl, lv_color_white(), 0);
        lv_obj_align(icon_lbl, LV_ALIGN_CENTER, 0, -12);

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
    Serial.println("[UI] launcher carousel OK");
}

void ui_launcher_show() {
    if (_screen) lv_scr_load_anim(_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
}

void ui_launcher_btn_tick() {}
