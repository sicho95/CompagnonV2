// ============================================================
// CompagnonV2 — ui/launcher.cpp
// Carousel 2 pages + navigation physique (KEY3 / BOOT)
//
// Boutons (PIN_KEY3=GPIO18, PIN_BOOT_BTN=GPIO0) :
//   KEY3  court  → page suivante (droite)   [seulement si launcher visible]
//   BOOT  court  → page précédente (gauche) [seulement si launcher visible]
//   KEY3  long   → valider (lancer app sélectionnée) [seulement si launcher visible]
//   BOOT  long   → fermer app courante (global, même si launcher pas visible)
//
// Labels ASCII pour eviter tout probleme d'encodage police.
// ============================================================
#include "launcher.h"
#include "status_bar.h"
#include "../hal/display.h"
#include "../hal/pmu.h"
#include "../system/os_kernel.h"
#include "../../include/pins.h"
#include <lvgl.h>
#include <Arduino.h>

#define STATUS_BAR_H  28
#define DOT_AREA_H    18
#define LONG_PRESS_MS 600

struct TileDesc {
    os::AppId   id;
    const char* icon;
    const char* label;
};

// Labels 100% ASCII — evite tout probleme de glyphe / encodage
static const TileDesc PAGE0[3] = {
    { os::AppId::NESTOR,  LV_SYMBOL_AUDIO,    "Nestor"   },
    { os::AppId::METEO,   LV_SYMBOL_HOME,     "Meteo"    },
    { os::AppId::BOURSE,  LV_SYMBOL_CHARGE,   "Bourse"   },
};
static const TileDesc PAGE1[3] = {
    { os::AppId::RADARS,  LV_SYMBOL_EYE_OPEN, "Radars"   },
    { os::AppId::RAPPELS, LV_SYMBOL_BELL,     "Rappels"  },
    { os::AppId::NONE,    LV_SYMBOL_SETTINGS, "Reglages" },
};

static lv_obj_t* _screen   = nullptr;
static lv_obj_t* _tileview = nullptr;
static lv_obj_t* _dot[2]   = {};
static int       _cur_page  = 0;
static int       _cur_card  = 0;
static int       _num_pages = 2;

static lv_obj_t* _cards[2][3] = {};

static const lv_color_t DOT_ACTIVE   = LV_COLOR_MAKE(0x4F, 0xC3, 0xF7);
static const lv_color_t DOT_INACTIVE = LV_COLOR_MAKE(0x33, 0x33, 0x55);
static const lv_color_t CARD_SEL     = LV_COLOR_MAKE(0x1A, 0x6A, 0xA0);
static const lv_color_t CARD_NORMAL  = LV_COLOR_MAKE(0x1C, 0x1C, 0x2E);

static void _update_dots(int page) {
    for (int i = 0; i < _num_pages; i++)
        lv_obj_set_style_bg_color(_dot[i],
            (i == page) ? DOT_ACTIVE : DOT_INACTIVE, 0);
    _cur_page = page;
}

static void _highlight_card(int page, int card) {
    for (int p = 0; p < _num_pages; p++)
        for (int c = 0; c < 3; c++)
            if (_cards[p][c])
                lv_obj_set_style_bg_color(_cards[p][c],
                    (p == page && c == card) ? CARD_SEL : CARD_NORMAL, 0);
    _cur_card = card;
}

static void _go_to_page(int page) {
    if (!_tileview) return;
    if (page < 0) page = 0;
    if (page >= _num_pages) page = _num_pages - 1;
    uint32_t n = lv_obj_get_child_count(_tileview);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t* child = lv_obj_get_child(_tileview, (int32_t)i);
        if ((int)(intptr_t)lv_obj_get_user_data(child) == page) {
            lv_tileview_set_tile(_tileview, child, LV_ANIM_ON);
            break;
        }
    }
    _update_dots(page);
    _highlight_card(page, _cur_card);
}

static void _tileview_changed_cb(lv_event_t* e) {
    lv_obj_t* tv   = (lv_obj_t*)lv_event_get_target(e);
    lv_obj_t* tile = lv_tileview_get_tile_active(tv);
    int page = (int)(intptr_t)lv_obj_get_user_data(tile);
    _update_dots(page);
    _highlight_card(page, _cur_card);
}

static void _launch_current() {
    const TileDesc* descs = (_cur_page == 0) ? PAGE0 : PAGE1;
    const TileDesc& d = descs[_cur_card];
    if (d.id != os::AppId::NONE) {
        Serial.printf("[UI] launch app %d (btn)\n", (int)d.id);
        os::app_launch(d.id);
    } else {
        Serial.println("[UI] Reglages (TODO)");
    }
}

static void _tile_tap_cb(lv_event_t* e) {
    auto* desc = (const TileDesc*)lv_event_get_user_data(e);
    if (!desc) return;
    if (desc->id != os::AppId::NONE)
        os::app_launch(desc->id);
    else
        Serial.println("[UI] Reglages (TODO)");
}

static void _build_page(lv_obj_t* tv_tile, const TileDesc descs[3], int page_idx) {
    lv_obj_t* cont = lv_obj_create(tv_tile);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(cont, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 6, 0);
    lv_obj_set_style_pad_gap(cont, 8, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    static lv_coord_t col_dsc[] = {
        LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
        LV_GRID_TEMPLATE_LAST
    };
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
        lv_obj_set_style_bg_color(card, CARD_NORMAL, 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(card, 16, 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x2A2A40), 0);
        lv_obj_set_style_pad_all(card, 0, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x2E2E50), LV_STATE_PRESSED);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);

        _cards[page_idx][i] = card;

        lv_obj_t* icon = lv_label_create(card);
        lv_label_set_text(icon, descs[i].icon);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(icon, lv_color_white(), 0);
        lv_obj_align(icon, LV_ALIGN_CENTER, 0, -12);

        lv_obj_t* lbl = lv_label_create(card);
        lv_label_set_text(lbl, descs[i].label);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xCCCCDD), 0);
        lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, -10);

        lv_obj_add_event_cb(card, _tile_tap_cb, LV_EVENT_CLICKED,
                            (void*)&descs[i]);
    }
}

static uint32_t _key3_down_ms  = 0;
static uint32_t _boot_down_ms  = 0;
static bool     _key3_was_low  = false;
static bool     _boot_was_low  = false;

void ui_launcher_btn_tick() {
    // NOTE : pas de guard "lv_scr_act() == _screen" ici.
    // BOOT long doit fonctionner même quand une app est active (écran != launcher).
    // La condition launcher_active protège KEY3 et BOOT court (navigation carousel)
    // pour éviter des changements de page fantômes quand le launcher est masqué.

    if (!_screen) return;
    bool launcher_active = (lv_scr_act() == _screen);

    uint32_t now = millis();

    // —— KEY3 (GPIO18) ——
    bool key3_low = (digitalRead(PIN_KEY3) == LOW);
    if (key3_low && !_key3_was_low) {
        _key3_down_ms = now;
    } else if (!key3_low && _key3_was_low) {
        if (launcher_active) {
            uint32_t held = now - _key3_down_ms;
            if (held >= LONG_PRESS_MS) {
                // Long : lancer l'app sélectionnée
                _launch_current();
            } else {
                // Court : page suivante (droite)
                int next = (_cur_page + 1) % _num_pages;
                _go_to_page(next);
            }
        }
    }
    _key3_was_low = key3_low;

    // —— BOOT (GPIO0) ——
    bool boot_low = (digitalRead(PIN_BOOT_BTN) == LOW);
    if (boot_low && !_boot_was_low) {
        _boot_down_ms = now;
    } else if (!boot_low && _boot_was_low) {
        uint32_t held = now - _boot_down_ms;
        if (held >= LONG_PRESS_MS) {
            // Long : fermer l'app courante (global — fonctionne hors launcher)
            if (os::app_current() != os::AppId::NONE) {
                Serial.println("[UI] BOOT long -> app_close_current");
                os::app_close_current();
            } else {
                Serial.println("[UI] BOOT long -> rien (launcher actif)");
            }
        } else if (launcher_active) {
            // Court : page précédente (gauche), sens opposé à KEY3
            int prev = (_cur_page - 1 + _num_pages) % _num_pages;
            _go_to_page(prev);
        }
    }
    _boot_was_low = boot_low;
}

void ui_launcher_init() {
    pinMode(PIN_KEY3,     INPUT_PULLUP);
    pinMode(PIN_BOOT_BTN, INPUT_PULLUP);

    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(_screen, LV_OBJ_FLAG_SCROLLABLE);

    int32_t tv_y = STATUS_BAR_H + 4;
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

    lv_obj_t* page0 = lv_tileview_add_tile(_tileview, 0, 0, LV_DIR_HOR);
    lv_obj_set_user_data(page0, (void*)(intptr_t)0);
    _build_page(page0, PAGE0, 0);

    lv_obj_t* page1 = lv_tileview_add_tile(_tileview, 1, 0, LV_DIR_HOR);
    lv_obj_set_user_data(page1, (void*)(intptr_t)1);
    _build_page(page1, PAGE1, 1);

    lv_obj_add_event_cb(_tileview, _tileview_changed_cb,
                        LV_EVENT_VALUE_CHANGED, nullptr);

    int32_t dot_y       = LV_VER_RES - DOT_AREA_H + (DOT_AREA_H - 8) / 2;
    int32_t dot_spacing = 14;
    int32_t dots_total  = _num_pages * 8 + (_num_pages - 1) * dot_spacing;
    int32_t dot_x_start = (LV_HOR_RES - dots_total) / 2;

    for (int i = 0; i < _num_pages; i++) {
        _dot[i] = lv_obj_create(_screen);
        lv_obj_set_size(_dot[i], 8, 8);
        lv_obj_set_pos(_dot[i], dot_x_start + i * (8 + dot_spacing), dot_y);
        lv_obj_set_style_radius(_dot[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(_dot[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(_dot[i], 0, 0);
        lv_obj_clear_flag(_dot[i], LV_OBJ_FLAG_SCROLLABLE);
    }
    _highlight_card(0, 0);
    _update_dots(0);

    lv_scr_load(_screen);
    ui_status_bar_raise();
    hal::display_force_refresh();
    Serial.println("[UI] launcher carousel OK");
}

void ui_launcher_show() {
    if (_screen) {
        lv_scr_load(_screen);
        ui_status_bar_raise();
        lv_obj_invalidate(lv_scr_act());
    }
}
