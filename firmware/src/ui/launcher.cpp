// ============================================================
// CompagnonV2 — ui/launcher.cpp
// Launcher pagine type montre: grille 3x2, 6 icones par page.
//
// Boutons:
//   KEY3 court  -> selection suivante
//   BOOT court  -> selection precedente
//   KEY3 long   -> lancer l'app selectionnee
//   BOOT long   -> fermer l'app courante
// ============================================================
#include "launcher.h"
#include "status_bar.h"
#include "../hal/display.h"
#include "../hal/touch.h"
#include "../system/os_kernel.h"
#include "../config/nvs_config.h"
#include "../../include/pins.h"
#include <lvgl.h>
#include <Arduino.h>

#define STATUS_BAR_H   28
#define FOOTER_H       28
#define PAGE_SIZE      6
#define GRID_COLS      3
#define GRID_ROWS      2
#define LONG_PRESS_MS  600
#define SWIPE_THRESHOLD_PX  44
#define SWIPE_AXIS_SLOP_PX  18
#define HOTSPOT_W      126
#define HOTSPOT_H      170
#define ICON_WELL_SZ   104

struct TileDesc {
    os::AppId   id;
    const char* icon;
    const char* label;
};

static constexpr TileDesc APPS[] = {
    { os::AppId::NESTOR,  "AI",               "Nestor"   },
    { os::AppId::METEO,   LV_SYMBOL_TINT,     "Meteo"    },
    { os::AppId::BOURSE,  LV_SYMBOL_BARS,     "Bourse"   },
    { os::AppId::RADARS,  LV_SYMBOL_WIFI,     "Radars"   },
    { os::AppId::RAPPELS, LV_SYMBOL_BELL,     "Rappels"  },
    { os::AppId::NONE,    LV_SYMBOL_SETTINGS, "Reglages" },
};

static constexpr int APP_COUNT = (int)(sizeof(APPS) / sizeof(APPS[0]));
static constexpr int PAGE_COUNT = (APP_COUNT + PAGE_SIZE - 1) / PAGE_SIZE;

static lv_obj_t* _screen = nullptr;
static lv_obj_t* _pages[PAGE_COUNT] = {};
static lv_obj_t* _slots[PAGE_COUNT][PAGE_SIZE] = {};
static lv_obj_t* _cards[PAGE_COUNT][PAGE_SIZE] = {};
static lv_obj_t* _icon_wells[PAGE_COUNT][PAGE_SIZE] = {};
static lv_obj_t* _labels[PAGE_COUNT][PAGE_SIZE] = {};
static lv_obj_t* _dots[PAGE_COUNT] = {};
static lv_obj_t* _bg_objs[16] = {};
static uint8_t   _bg_obj_count = 0;
static uint8_t   _bg_style_loaded = 0xFF;

struct TileEventCtx {
    int  linear = -1;
    // press_lost conservé pour debug éventuel mais ne bloque plus le click
    bool press_lost = false;
};
static TileEventCtx _tile_ctx[PAGE_COUNT][PAGE_SIZE] = {};

static int _cur_page = 0;
static int _cur_slot = 0;
static bool _gesture_track = false;
static bool _gesture_swipe_locked = false;
static bool _gesture_click_suppressed = false;
static bool _gesture_status_origin = false;
static int16_t _gesture_start_x = 0;
static int16_t _gesture_start_y = 0;

static const lv_color_t BG_TOP       = LV_COLOR_MAKE(0x00, 0x00, 0x00);
static const lv_color_t BG_BOTTOM    = LV_COLOR_MAKE(0x00, 0x00, 0x00);
static const lv_color_t CARD_ACTIVE  = LV_COLOR_MAKE(0x22, 0x2D, 0x4A);
static const lv_color_t WELL_IDLE    = LV_COLOR_MAKE(0x98, 0xA3, 0xB5);
static const lv_color_t WELL_ACTIVE  = LV_COLOR_MAKE(0x66, 0xC7, 0xFF);
static const lv_color_t LABEL_IDLE   = LV_COLOR_MAKE(0xDC, 0xE5, 0xF2);
static const lv_color_t LABEL_ACTIVE = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF);
static const lv_color_t DOT_IDLE     = LV_COLOR_MAKE(0x45, 0x52, 0x70);
static const lv_color_t DOT_ACTIVE   = LV_COLOR_MAKE(0xE8, 0xF3, 0xFF);

static void _tile_event_cb(lv_event_t* e);

static void _bg_track(lv_obj_t* obj) {
    if (_bg_obj_count < (sizeof(_bg_objs) / sizeof(_bg_objs[0]))) {
        _bg_objs[_bg_obj_count++] = obj;
    }
}

static lv_obj_t* _bg_circle(lv_obj_t* parent, int x, int y, int sz,
                            lv_color_t color, lv_opa_t opa, int border_w) {
    lv_obj_t* o = lv_obj_create(parent);
    _bg_track(o);
    lv_obj_set_size(o, sz, sz);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(o, border_w, 0);
    lv_obj_set_style_border_color(o, color, 0);
    lv_obj_set_style_border_opa(o, opa, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_background(o);
    return o;
}

static lv_obj_t* _bg_rect(lv_obj_t* parent, int x, int y, int w, int h,
                          lv_color_t color, lv_opa_t opa, int radius) {
    lv_obj_t* o = lv_obj_create(parent);
    _bg_track(o);
    lv_obj_set_size(o, w, h);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_set_style_bg_color(o, color, 0);
    lv_obj_set_style_bg_opa(o, opa, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_background(o);
    return o;
}

static void _clear_background() {
    Serial.printf("[UI] launcher bg clear count=%u\n", _bg_obj_count);
    for (uint8_t i = 0; i < _bg_obj_count; ++i) {
        if (_bg_objs[i]) lv_obj_delete(_bg_objs[i]);
        _bg_objs[i] = nullptr;
    }
    _bg_obj_count = 0;
}

static void _build_background(uint8_t style_id) {
    Serial.printf("[UI] launcher bg build style=%u prev=%u\n",
                  style_id, _bg_style_loaded);
    _clear_background();
    uint8_t style = style_id % 3;
    if (style == 0) {
        _bg_circle(_screen, 50, 70, 380, lv_color_hex(0x2E82FF), LV_OPA_30, 2);
        _bg_circle(_screen, 92, 112, 296, lv_color_hex(0x67C3FF), LV_OPA_20, 2);
        _bg_circle(_screen, 132, 152, 216, lv_color_hex(0x87E0FF), LV_OPA_30, 2);
        _bg_rect(_screen, 238, 70, 2, 360, lv_color_hex(0x4FA9FF), LV_OPA_20, 0);
        _bg_rect(_screen, 60, 248, 360, 2, lv_color_hex(0x4FA9FF), LV_OPA_20, 0);
    } else if (style == 1) {
        _bg_rect(_screen, 56, 110, 368, 4, lv_color_hex(0x72CAFF), LV_OPA_30, 2);
        _bg_rect(_screen, 56, 370, 368, 4, lv_color_hex(0x72CAFF), LV_OPA_20, 2);
        _bg_rect(_screen, 112, 64, 4, 352, lv_color_hex(0x72CAFF), LV_OPA_20, 2);
        _bg_rect(_screen, 364, 64, 4, 352, lv_color_hex(0x72CAFF), LV_OPA_20, 2);
        _bg_circle(_screen, 148, 148, 184, lv_color_hex(0xC8F2FF), LV_OPA_20, 1);
        _bg_circle(_screen, 180, 180, 120, lv_color_hex(0xA2E8FF), LV_OPA_20, 1);
    } else {
        _bg_circle(_screen, -120, -60, 260, lv_color_hex(0x1A8CFF), LV_OPA_20, 3);
        _bg_circle(_screen, 342, -40, 220, lv_color_hex(0x1A8CFF), LV_OPA_20, 3);
        _bg_circle(_screen, -110, 272, 250, lv_color_hex(0x1A8CFF), LV_OPA_20, 3);
        _bg_circle(_screen, 346, 294, 220, lv_color_hex(0x1A8CFF), LV_OPA_20, 3);
        _bg_rect(_screen, 46, 52, 388, 376, lv_color_hex(0x0A1632), LV_OPA_10, 34);
    }
    _bg_style_loaded = style;
    Serial.printf("[UI] launcher bg build done count=%u\n", _bg_obj_count);
}

static int _page_start_index(int page) {
    return page * PAGE_SIZE;
}

static int _page_item_count(int page) {
    int remain = APP_COUNT - _page_start_index(page);
    return remain > PAGE_SIZE ? PAGE_SIZE : remain;
}

static int _linear_index() {
    return _page_start_index(_cur_page) + _cur_slot;
}

static void _set_page_visible(int page) {
    for (int i = 0; i < PAGE_COUNT; ++i) {
        if (_pages[i]) {
            if (i == page) lv_obj_clear_flag(_pages[i], LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(_pages[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void _update_dots() {
    for (int i = 0; i < PAGE_COUNT; ++i) {
        if (!_dots[i]) continue;
        lv_obj_set_style_bg_color(_dots[i], i == _cur_page ? DOT_ACTIVE : DOT_IDLE, 0);
        lv_obj_set_width(_dots[i], i == _cur_page ? 18 : 8);
    }
}

static void _highlight_selection() {
    for (int p = 0; p < PAGE_COUNT; ++p) {
        int count = _page_item_count(p);
        for (int i = 0; i < count; ++i) {
            lv_obj_t* card = _cards[p][i];
            lv_obj_t* well = _icon_wells[p][i];
            if (!card || !well) continue;

            bool selected = (p == _cur_page && i == _cur_slot);
            lv_obj_set_style_bg_color(card, CARD_ACTIVE, 0);
            lv_obj_set_style_bg_opa(card, selected ? LV_OPA_10 : LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(card, 0, 0);
            lv_obj_set_style_shadow_width(card, 0, 0);

            lv_obj_set_style_bg_color(well, selected ? WELL_ACTIVE : WELL_IDLE, 0);
            lv_obj_set_style_text_color(well,
                                        selected ? lv_color_hex(0x08111F) : lv_color_hex(0x1A2540),
                                        0);
            lv_obj_set_style_shadow_width(well, selected ? 22 : 0, 0);
            lv_obj_set_style_shadow_opa(well, selected ? LV_OPA_30 : LV_OPA_TRANSP, 0);
            lv_obj_set_style_shadow_color(well, lv_color_hex(0x5CCFFF), 0);
            if (_labels[p][i]) {
                lv_obj_set_style_text_color(_labels[p][i],
                                            selected ? LABEL_ACTIVE : LABEL_IDLE, 0);
            }
        }
    }
}

static void _sync_selection() {
    _set_page_visible(_cur_page);
    _update_dots();
    _highlight_selection();
    lv_obj_invalidate(_screen);
    hal::display_force_refresh();
    Serial.printf("[UI] launcher sel page=%d slot=%d idx=%d\n",
                  _cur_page, _cur_slot, _linear_index());
}

static void _set_selection_from_linear(int linear) {
    const int total = APP_COUNT;
    if (linear < 0) linear = total - 1;
    if (linear >= total) linear = 0;
    _cur_page = linear / PAGE_SIZE;
    _cur_slot = linear % PAGE_SIZE;
    _sync_selection();
}

static void _move_selection(int delta) {
    _set_selection_from_linear(_linear_index() + delta);
}

static void _move_page(int delta) {
    int next_page = _cur_page + delta;
    if (next_page < 0) next_page = PAGE_COUNT - 1;
    if (next_page >= PAGE_COUNT) next_page = 0;

    int next_count = _page_item_count(next_page);
    if (_cur_slot >= next_count) _cur_slot = next_count - 1;
    if (_cur_slot < 0) _cur_slot = 0;
    _cur_page = next_page;
    _sync_selection();
    Serial.printf("[UI] launcher swipe -> page=%d slot=%d\n", _cur_page, _cur_slot);
}

static void _launch_desc(const TileDesc& d, const char* source) {
    if (d.id != os::AppId::NONE) {
        Serial.printf("[UI] launch app %d (%s)\n", (int)d.id, source);
        os::app_launch(d.id);
    } else {
        Serial.println("[UI] Reglages (TODO)");
    }
}

static void _launch_current() {
    _launch_desc(APPS[_linear_index()], "btn");
}

static lv_obj_t* _make_icon_well(lv_obj_t* parent, const char* icon) {
    lv_obj_t* well = lv_obj_create(parent);
    lv_obj_set_size(well, ICON_WELL_SZ, ICON_WELL_SZ);
    lv_obj_set_style_radius(well, 26, 0);
    lv_obj_set_style_border_width(well, 0, 0);
    lv_obj_set_style_pad_all(well, 0, 0);
    lv_obj_set_style_bg_opa(well, LV_OPA_COVER, 0);
    lv_obj_clear_flag(well, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_shadow_width(well, 0, 0);

    lv_obj_t* icon_lbl = lv_label_create(well);
    lv_label_set_text(icon_lbl, icon);
    lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_30, 0);
    lv_obj_center(icon_lbl);
    return well;
}

static void _build_page(lv_obj_t* parent, int page) {
    lv_obj_t* page_obj = lv_obj_create(parent);
    _pages[page] = page_obj;
    lv_obj_set_size(page_obj, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(page_obj, 0, 0);
    lv_obj_set_style_bg_opa(page_obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(page_obj, 0, 0);
    lv_obj_set_style_pad_left(page_obj, 10, 0);
    lv_obj_set_style_pad_right(page_obj, 10, 0);
    lv_obj_set_style_pad_top(page_obj, 10, 0);
    lv_obj_set_style_pad_bottom(page_obj, 6, 0);
    lv_obj_set_style_pad_column(page_obj, 6, 0);
    lv_obj_set_style_pad_row(page_obj, 6, 0);
    lv_obj_clear_flag(page_obj, LV_OBJ_FLAG_SCROLLABLE);

    static lv_coord_t col_dsc[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static lv_coord_t row_dsc[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    lv_obj_set_grid_dsc_array(page_obj, col_dsc, row_dsc);
    lv_obj_set_layout(page_obj, LV_LAYOUT_GRID);

    int start = _page_start_index(page);
    int count = _page_item_count(page);
    for (int i = 0; i < count; ++i) {
        const TileDesc& desc = APPS[start + i];
        int col = i % GRID_COLS;
        int row = i / GRID_COLS;

        lv_obj_t* slot = lv_obj_create(page_obj);
        _slots[page][i] = slot;
        lv_obj_set_grid_cell(slot,
                             LV_GRID_ALIGN_STRETCH, col, 1,
                             LV_GRID_ALIGN_STRETCH, row, 1);
        lv_obj_set_style_bg_opa(slot, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(slot, 0, 0);
        lv_obj_set_style_pad_all(slot, 0, 0);
        lv_obj_clear_flag(slot, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* card = lv_obj_create(slot);
        _cards[page][i] = card;
        lv_obj_set_size(card, HOTSPOT_W, HOTSPOT_H);
        lv_obj_center(card);
        lv_obj_set_style_radius(card, 18, 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(card, 0, 0);
        lv_obj_set_style_pad_top(card, 6, 0);
        lv_obj_set_style_pad_bottom(card, 6, 0);
        lv_obj_set_style_pad_left(card, 4, 0);
        lv_obj_set_style_pad_right(card, 4, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(card, LV_OBJ_FLAG_EVENT_BUBBLE);
        _tile_ctx[page][i].linear = start + i;
        _tile_ctx[page][i].press_lost = false;
        lv_obj_add_event_cb(card, _tile_event_cb, LV_EVENT_PRESSED, &_tile_ctx[page][i]);
        lv_obj_add_event_cb(card, _tile_event_cb, LV_EVENT_PRESS_LOST, &_tile_ctx[page][i]);
        lv_obj_add_event_cb(card, _tile_event_cb, LV_EVENT_RELEASED, &_tile_ctx[page][i]);
        lv_obj_add_event_cb(card, _tile_event_cb, LV_EVENT_CLICKED, &_tile_ctx[page][i]);

        lv_obj_t* col_cont = lv_obj_create(card);
        lv_obj_set_size(col_cont, LV_PCT(100), LV_PCT(100));
        lv_obj_center(col_cont);
        lv_obj_set_style_bg_opa(col_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(col_cont, 0, 0);
        lv_obj_set_style_pad_all(col_cont, 0, 0);
        lv_obj_set_flex_flow(col_cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(col_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(col_cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(col_cont, LV_OBJ_FLAG_EVENT_BUBBLE);

        lv_obj_t* well = _make_icon_well(col_cont, desc.icon);
        _icon_wells[page][i] = well;

        lv_obj_t* lbl = lv_label_create(col_cont);
        _labels[page][i] = lbl;
        lv_label_set_text(lbl, desc.label);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl, LABEL_IDLE, 0);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(lbl, LV_PCT(100));
        lv_obj_set_style_pad_top(lbl, 10, 0);
        lv_obj_set_style_max_width(lbl, HOTSPOT_W - 8, 0);

        lv_obj_add_flag(well, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_flag(lbl, LV_OBJ_FLAG_EVENT_BUBBLE);
    }
}

static void _tile_event_cb(lv_event_t* e) {
    TileEventCtx* ctx = static_cast<TileEventCtx*>(lv_event_get_user_data(e));
    if (!ctx || ctx->linear < 0 || ctx->linear >= APP_COUNT) return;
    if (!_screen || lv_scr_act() != _screen) return;

    switch (lv_event_get_code(e)) {
        case LV_EVENT_PRESSED:
            ctx->press_lost = false;
            _set_selection_from_linear(ctx->linear);
            break;
        case LV_EVENT_PRESS_LOST:
            ctx->press_lost = true;
            break;
        case LV_EVENT_RELEASED:
            break;
        case LV_EVENT_CLICKED:
            // Fix: on retire la condition ctx->press_lost.
            // LV_EVENT_CLICKED est garanti par LVGL uniquement si le doigt
            // se relache sur l'objet — pas besoin de double-filtrer.
            // press_lost bloquait le launch quand un getPoint() invalide
            // survenait pendant le press (IRQ retombee trop tôt).
            if (!_gesture_click_suppressed) {
                _set_selection_from_linear(ctx->linear);
                _launch_desc(APPS[ctx->linear], "touch");
            }
            break;
        default:
            break;
    }
}

static void _build_footer(lv_obj_t* parent) {
    int32_t dot_spacing = 10;
    int32_t total_w = PAGE_COUNT * 8 + (PAGE_COUNT - 1) * dot_spacing;
    int32_t x = LCD_SAFE_X + (LCD_SAFE_WIDTH - total_w) / 2;
    int32_t y = LCD_SAFE_Y + LCD_SAFE_HEIGHT - FOOTER_H + 8;

    for (int i = 0; i < PAGE_COUNT; ++i) {
        _dots[i] = lv_obj_create(parent);
        lv_obj_set_size(_dots[i], 8, 8);
        lv_obj_set_pos(_dots[i], x + i * (8 + dot_spacing), y);
        lv_obj_set_style_radius(_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(_dots[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(_dots[i], 0, 0);
        lv_obj_clear_flag(_dots[i], LV_OBJ_FLAG_SCROLLABLE);
    }
}

static uint32_t _key3_down_ms = 0;
static uint32_t _boot_down_ms = 0;
static bool _key3_was_low = false;
static bool _boot_was_low = false;

void ui_launcher_btn_tick() {
    if (!_screen) return;
    bool launcher_active = (lv_scr_act() == _screen);
    uint32_t now = millis();

    bool key3_low = (digitalRead(PIN_KEY3) == LOW);
    if (key3_low && !_key3_was_low) {
        _key3_down_ms = now;
    } else if (!key3_low && _key3_was_low) {
        if (launcher_active) {
            uint32_t held = now - _key3_down_ms;
            if (held >= LONG_PRESS_MS) _launch_current();
            else _move_selection(+1);
        }
    }
    _key3_was_low = key3_low;

    bool boot_low = (digitalRead(PIN_BOOT_BTN) == LOW);
    if (boot_low && !_boot_was_low) {
        _boot_down_ms = now;
    } else if (!boot_low && _boot_was_low) {
        uint32_t held = now - _boot_down_ms;
        if (held >= LONG_PRESS_MS) {
            if (os::app_current() != os::AppId::NONE) {
                Serial.println("[UI] BOOT long -> app_close_current");
                os::app_close_current();
            } else {
                Serial.println("[UI] BOOT long -> rien (launcher actif)");
            }
        } else if (launcher_active) {
            _move_selection(-1);
        }
    }
    _boot_was_low = boot_low;
}

void ui_launcher_touch_tick() {
    // IMPORTANT : cette fonction doit être appelée APRES lv_timer_handler()
    // dans la task LVGL. Si elle est appelée avant, le frame touch est
    // consommé/reseté avant que LVGL puisse dispatcher LV_EVENT_CLICKED.
    if (!_screen) return;
    const bool launcher_active = (lv_scr_act() == _screen);
    const hal::TouchFrame& frame = hal::touch_frame();

    if (!launcher_active) {
        _gesture_track = false;
        _gesture_swipe_locked = false;
        _gesture_click_suppressed = false;
        _gesture_status_origin = false;
        return;
    }

    if (frame.just_pressed && frame.point_count > 0 && frame.points[0].valid) {
        _gesture_track = true;
        _gesture_swipe_locked = false;
        _gesture_click_suppressed = false;
        _gesture_status_origin = (frame.points[0].y >= LCD_SAFE_Y &&
                                  frame.points[0].y < (LCD_SAFE_Y + STATUS_BAR_H));
        _gesture_start_x = frame.points[0].x;
        _gesture_start_y = frame.points[0].y;
        return;
    }

    if (_gesture_track && frame.pressed && frame.point_count > 0 && frame.points[0].valid && !_gesture_swipe_locked) {
        if (_gesture_status_origin) return;

        const int32_t dx = (int32_t)frame.points[0].x - (int32_t)_gesture_start_x;
        const int32_t dy = (int32_t)frame.points[0].y - (int32_t)_gesture_start_y;
        const int32_t adx = dx >= 0 ? dx : -dx;
        const int32_t ady = dy >= 0 ? dy : -dy;

        if (adx >= SWIPE_THRESHOLD_PX && adx > (ady + SWIPE_AXIS_SLOP_PX)) {
            _gesture_swipe_locked = true;
            _gesture_click_suppressed = true;
            _move_page(dx < 0 ? +1 : -1);
        }
        return;
    }

    if (frame.just_released || !frame.pressed) {
        _gesture_track = false;
        _gesture_swipe_locked = false;
        _gesture_click_suppressed = false;
        _gesture_status_origin = false;
    }
}

void ui_launcher_init() {
    pinMode(PIN_KEY3, INPUT_PULLUP);
    pinMode(PIN_BOOT_BTN, INPUT_PULLUP);

    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, BG_TOP, 0);
    lv_obj_set_style_bg_grad_color(_screen, BG_BOTTOM, 0);
    lv_obj_set_style_bg_grad_dir(_screen, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_screen, 0, 0);
    lv_obj_clear_flag(_screen, LV_OBJ_FLAG_SCROLLABLE);
    _build_background(cfg_get_u8(NVS_KEY_LAUNCHER_BG, 0));

    lv_obj_t* content = lv_obj_create(_screen);
    lv_obj_set_pos(content, LCD_SAFE_X, LCD_SAFE_Y + STATUS_BAR_H + 2);
    lv_obj_set_size(content, LCD_SAFE_WIDTH,
                    LCD_SAFE_HEIGHT - STATUS_BAR_H - FOOTER_H - 2);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    for (int page = 0; page < PAGE_COUNT; ++page) {
        _build_page(content, page);
    }

    _build_footer(_screen);
    _set_selection_from_linear(0);

    lv_scr_load(_screen);
    ui_status_bar_raise();
    hal::display_force_refresh();
    Serial.println("[UI] launcher grid OK");
}

void ui_launcher_show() {
    if (_screen) {
        Serial.printf("[UI] launcher show scr=%p active_before=%p\n",
                      _screen, lv_scr_act());
        uint8_t style = cfg_get_u8(NVS_KEY_LAUNCHER_BG, 0) % 3;
        if (_bg_style_loaded != style) {
            Serial.println("[UI] launcher show -> bg rebuild needed");
            _build_background(style);
        } else {
            Serial.println("[UI] launcher show -> bg reuse");
        }
        Serial.println("[UI] launcher show -> load screen");
        lv_scr_load(_screen);
        Serial.println("[UI] launcher show -> raise status");
        ui_status_bar_raise();
        Serial.println("[UI] launcher show -> sync selection");
        _sync_selection();
        lv_obj_invalidate(lv_scr_act());
        hal::display_force_refresh();
        Serial.printf("[UI] launcher show done active_after=%p\n", lv_scr_act());
    }
}
