// ============================================================
// CompagnonV2 — ui/launcher.cpp
// fix encodage   : LV_SYMBOL_* (glyphes intégrés LVGL) +
//                  labels UTF-8 en raw string
// fix status_bar : ui_status_bar_raise() après lv_scr_load()
// fix grille     : 2 pages × 3 colonnes par page (2×3)
//                  avec titre "CompagnonV2" centré ET status bar
// fix navigation : tuiles clickables avec lancement app
// fix compilation: LV_SYMBOL_CHART → LV_SYMBOL_CHARGE (n'existe pas dans LVGL)
// ============================================================
#include "launcher.h"
#include "status_bar.h"
#include "../hal/display.h"
#include "../system/os_kernel.h"
#include <lvgl.h>
#include <Arduino.h>

#define STATUS_BAR_H  28
#define DOT_AREA_H    18
#define TITLE_H       28

struct TileDesc {
    os::AppId   id;
    const char* icon;   // LV_SYMBOL_* — glyphe intégré LVGL
    const char* label;  // UTF-8
};

// LV_SYMBOL_* : glyphes toujours disponibles dans la police built-in LVGL
// LV_SYMBOL_CHART n'existe PAS dans LVGL — utiliser LV_SYMBOL_CHARGE à la place
static const TileDesc PAGE0[3] = {
    { os::AppId::NESTOR,  LV_SYMBOL_AUDIO,    "Nestor"  },
    { os::AppId::METEO,   LV_SYMBOL_HOME,     "M\xc3\xa9t\xc3\xa9o" },
    { os::AppId::BOURSE,  LV_SYMBOL_CHARGE,   "Bourse"  },
};
static const TileDesc PAGE1[3] = {
    { os::AppId::RADARS,  LV_SYMBOL_EYE_OPEN, "Radars"  },
    { os::AppId::RAPPELS, LV_SYMBOL_BELL,     "Rappels" },
    { os::AppId::NONE,    LV_SYMBOL_SETTINGS, "R\xc3\xa9glages" },
};

static lv_obj_t* _screen   = nullptr;
static lv_obj_t* _tileview = nullptr;
static lv_obj_t* _dot[2]   = {};
static int       _cur_page = 0;

static const lv_color_t DOT_ACTIVE   = LV_COLOR_MAKE(0x4F, 0xC3, 0xF7);
static const lv_color_t DOT_INACTIVE = LV_COLOR_MAKE(0x33, 0x33, 0x55);

static void _update_dots(int page) {
    for (int i = 0; i < 2; i++)
        lv_obj_set_style_bg_color(_dot[i],
            (i == page) ? DOT_ACTIVE : DOT_INACTIVE, 0);
    _cur_page = page;
}

static void _tileview_changed_cb(lv_event_t* e) {
    lv_obj_t* tv   = lv_event_get_target(e);
    lv_obj_t* tile = lv_tileview_get_tile_active(tv);
    int page = (int)(intptr_t)lv_obj_get_user_data(tile);
    _update_dots(page);
}

static void _tile_tap_cb(lv_event_t* e) {
    auto* desc = (const TileDesc*)lv_event_get_user_data(e);
    if (!desc) return;
    if (desc->id != os::AppId::NONE) {
        Serial.printf("[UI] launch app %d\n", (int)desc->id);
        os::app_launch(desc->id);
    } else {
        Serial.println("[UI] Reglages (TODO)");
    }
}

// Construit une page avec 3 tuiles en colonne
static void _build_page(lv_obj_t* tv_tile, const TileDesc descs[3]) {
    lv_obj_t* cont = lv_obj_create(tv_tile);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(cont, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 6, 0);
    lv_obj_set_style_pad_gap(cont, 8, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    // 3 colonnes egales
    static lv_coord_t col_dsc[] = {
        LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
        LV_GRID_TEMPLATE_LAST
    };
    // 1 rangee (les 3 tuiles de cette page)
    static lv_coord_t row_dsc[] = {
        LV_GRID_FR(1),
        LV_GRID_TEMPLATE_LAST
    };
    lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);

    for (int i = 0; i < 3; i++) {
        lv_obj_t* card = lv_obj_create(cont);
        lv_obj_set_size(card, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_grid_cell(card,
            LV_GRID_ALIGN_STRETCH, i, 1,
            LV_GRID_ALIGN_STRETCH, 0, 1);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x1C1C2E), 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(card, 16, 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x2A2A40), 0);
        lv_obj_set_style_pad_all(card, 0, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x2E2E50), LV_STATE_PRESSED);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);

        // Icone LV_SYMBOL (police built-in LVGL, toujours disponible)
        lv_obj_t* icon = lv_label_create(card);
        lv_label_set_text(icon, descs[i].icon);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(icon, lv_color_white(), 0);
        lv_obj_align(icon, LV_ALIGN_CENTER, 0, -12);

        // Label texte en bas
        lv_obj_t* lbl = lv_label_create(card);
        lv_label_set_text(lbl, descs[i].label);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xCCCCDD), 0);
        lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, -10);

        lv_obj_add_event_cb(card, _tile_tap_cb, LV_EVENT_CLICKED,
                            (void*)&descs[i]);
    }
}

void ui_launcher_init() {
    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(_screen, LV_OBJ_FLAG_SCROLLABLE);

    // Titre "CompagnonV2" centre en haut (sous la status bar)
    lv_obj_t* title = lv_label_create(_screen);
    lv_label_set_text(title, "CompagnonV2");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x7EB8D4), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, STATUS_BAR_H + 6);

    // TileView : commence apres status_bar + titre
    int32_t tv_y = STATUS_BAR_H + TITLE_H + 8;
    int32_t tv_h = LV_VER_RES - tv_y - DOT_AREA_H - 4;
    int32_t tv_w = LV_HOR_RES;

    _tileview = lv_tileview_create(_screen);
    lv_obj_set_pos(_tileview, 0, tv_y);
    lv_obj_set_size(_tileview, tv_w, tv_h);
    lv_obj_set_style_bg_opa(_tileview, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_tileview, 0, 0);
    lv_obj_set_style_pad_all(_tileview, 0, 0);
    lv_obj_set_scroll_dir(_tileview, LV_DIR_HOR);
    lv_obj_set_scrollbar_mode(_tileview, LV_SCROLLBAR_MODE_OFF);

    // Page 0 : Nestor, Meteo, Bourse
    lv_obj_t* page0 = lv_tileview_add_tile(_tileview, 0, 0, LV_DIR_HOR);
    lv_obj_set_user_data(page0, (void*)(intptr_t)0);
    _build_page(page0, PAGE0);

    // Page 1 : Radars, Rappels, Reglages
    lv_obj_t* page1 = lv_tileview_add_tile(_tileview, 1, 0, LV_DIR_HOR);
    lv_obj_set_user_data(page1, (void*)(intptr_t)1);
    _build_page(page1, PAGE1);

    lv_obj_add_event_cb(_tileview, _tileview_changed_cb,
                        LV_EVENT_VALUE_CHANGED, nullptr);

    // Points de pagination (dots) en bas
    int32_t dot_y       = LV_VER_RES - DOT_AREA_H + (DOT_AREA_H - 8) / 2;
    int32_t dot_spacing = 14;
    int32_t dots_total  = 2 * 8 + dot_spacing;
    int32_t dot_x_start = (LV_HOR_RES - dots_total) / 2;

    for (int i = 0; i < 2; i++) {
        _dot[i] = lv_obj_create(_screen);
        lv_obj_set_size(_dot[i], 8, 8);
        lv_obj_set_pos(_dot[i], dot_x_start + i * (8 + dot_spacing), dot_y);
        lv_obj_set_style_radius(_dot[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(_dot[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(_dot[i], 0, 0);
        lv_obj_clear_flag(_dot[i], LV_OBJ_FLAG_SCROLLABLE);
    }
    _update_dots(0);

    lv_scr_load(_screen);

    // fix status_bar : raise APRES lv_scr_load() pour qu'elle
    // reste au-dessus du nouveau screen charge
    ui_status_bar_raise();

    hal::display_force_refresh();
    Serial.println("[UI] launcher carousel OK");
}

void ui_launcher_show() {
    if (_screen) {
        lv_scr_load_anim(_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
        ui_status_bar_raise();
    }
}

void ui_launcher_btn_tick() {}
