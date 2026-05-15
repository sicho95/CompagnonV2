// ============================================================
// CompagnonV2 — ui/launcher.cpp
// Tileview 5 apps LVGL 8 avec navigation boutons + touch
// ============================================================
#include "launcher.h"
#include "../system/os_kernel.h"
#include "../hal/rtc.h"
#include "../../include/pins.h"
#include <lvgl.h>
#include <Arduino.h>

namespace ui {

// Apps dans l'ordre du carousel
static const struct { os::AppId id; const char* label; const char* icon; } _apps[] = {
    { os::AppId::NESTOR,  "Nestor",  LV_SYMBOL_AUDIO  },
    { os::AppId::RADARS,  "Radars",  LV_SYMBOL_WIFI   },
    { os::AppId::BOURSE,  "Bourse",  LV_SYMBOL_CHARGE },
    { os::AppId::METEO,   "Météo",   LV_SYMBOL_DRIVE  },
    { os::AppId::RAPPELS, "Rappels", LV_SYMBOL_BELL   },
};
static const int APP_COUNT = 5;

static lv_obj_t* _screen      = nullptr;
static lv_obj_t* _tileview    = nullptr;
static lv_obj_t* _tiles[APP_COUNT];
static int       _current_idx = 0;

// Boutons physiques — debounce
static uint32_t _btn_boot_down  = 0;
static uint32_t _btn_user_down  = 0;
static bool     _btn_boot_held  = false;
static bool     _btn_user_held  = false;
#define LONG_PRESS_MS 600

static void _build_tile(int i, lv_obj_t* tile) {
    lv_obj_set_style_bg_color(tile, lv_color_hex(0x111111), 0);
    lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

    // Icône
    lv_obj_t* icon = lv_label_create(tile);
    lv_label_set_text(icon, _apps[i].icon);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(icon, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -20);

    // Label
    lv_obj_t* lbl = lv_label_create(tile);
    lv_label_set_text(lbl, _apps[i].label);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 32);

    // Indicateur de position (points)
    for (int j = 0; j < APP_COUNT; j++) {
        lv_obj_t* dot = lv_obj_create(tile);
        lv_obj_set_size(dot, 8, 8);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot,
            j == i ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x444444), 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_align(dot, LV_ALIGN_BOTTOM_MID, (j - APP_COUNT/2) * 14, -12);
    }
}

void launcher_init() {
    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x0D0D0D), 0);

    _tileview = lv_tileview_create(_screen);
    lv_obj_set_size(_tileview, 480, LV_VER_RES - 36);
    lv_obj_align(_tileview, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(_tileview, lv_color_hex(0x0D0D0D), 0);

    for (int i = 0; i < APP_COUNT; i++) {
        _tiles[i] = lv_tileview_add_tile(_tileview, i, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
        _build_tile(i, _tiles[i]);
    }

    // GPIO boutons
    pinMode(PIN_BTN_BOOT, INPUT_PULLUP);
    pinMode(PIN_BTN_USER, INPUT_PULLUP);

    launcher_show();
    Serial.println("[LAUNCHER] init OK");
}

void launcher_show() {
    lv_scr_load(_screen);
    _current_idx = 0;
    lv_obj_set_tile_id(_tileview, 0, 0, LV_ANIM_ON);
}

void launcher_btn_next() {
    _current_idx = (_current_idx + 1) % APP_COUNT;
    lv_obj_set_tile_id(_tileview, _current_idx, 0, LV_ANIM_ON);
}

void launcher_btn_open() {
    os::app_launch(_apps[_current_idx].id);
}

void launcher_touch_swipe_left() {
    _current_idx = (_current_idx + 1) % APP_COUNT;
    lv_obj_set_tile_id(_tileview, _current_idx, 0, LV_ANIM_ON);
}

void launcher_touch_swipe_right() {
    _current_idx = (_current_idx - 1 + APP_COUNT) % APP_COUNT;
    lv_obj_set_tile_id(_tileview, _current_idx, 0, LV_ANIM_ON);
}

} // namespace ui
