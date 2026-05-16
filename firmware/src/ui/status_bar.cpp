// ============================================================
// CompagnonV2 — ui/status_bar.cpp
// Barre 480×36 LVGL 8 — AMOLED optimisé (fond noir pur)
// Icônes inactives : 0x111111 (pixels quasi-éteints)
// ============================================================
#include "status_bar.h"
#include "../hal/rtc.h"
#include "../hal/pmu.h"
#include "../net/wifi_mgr.h"
#include "../net/ble_mgr.h"
#include "../system/os_kernel.h"
#include <lvgl.h>
#include <time.h>
#include <stdio.h>

namespace ui {

static lv_obj_t* _bar      = nullptr;
static lv_obj_t* _lbl_time = nullptr;
static lv_obj_t* _lbl_ble  = nullptr;
static lv_obj_t* _lbl_wifi = nullptr;
static lv_obj_t* _lbl_batt = nullptr;
static lv_obj_t* _bar_batt = nullptr;

void status_bar_init() {
    _bar = lv_obj_create(lv_scr_act());
    lv_obj_set_size(_bar, 480, 36);
    lv_obj_align(_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(_bar, lv_color_black(), 0);  // AMOLED : noir pur
    lv_obj_set_style_bg_opa(_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_bar, 0, 0);
    lv_obj_set_style_radius(_bar, 0, 0);
    lv_obj_set_style_pad_all(_bar, 4, 0);
    lv_obj_clear_flag(_bar, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_time = lv_label_create(_bar);
    lv_obj_align(_lbl_time, LV_ALIGN_LEFT_MID, 6, 0);
    lv_obj_set_style_text_color(_lbl_time, lv_color_white(), 0);
    lv_obj_set_style_text_font(_lbl_time, &lv_font_montserrat_16, 0);
    lv_label_set_text(_lbl_time, "--:--");

    // BLE : déconnecté = 0x111111 (pixels quasi-éteints sur AMOLED)
    _lbl_ble = lv_label_create(_bar);
    lv_obj_align(_lbl_ble, LV_ALIGN_LEFT_MID, 100, 0);
    lv_obj_set_style_text_font(_lbl_ble, &lv_font_montserrat_16, 0);
    lv_label_set_text(_lbl_ble, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(_lbl_ble, lv_color_hex(0x111111), 0);

    // WiFi : déconnecté = 0x111111
    _lbl_wifi = lv_label_create(_bar);
    lv_obj_align(_lbl_wifi, LV_ALIGN_LEFT_MID, 130, 0);
    lv_obj_set_style_text_font(_lbl_wifi, &lv_font_montserrat_16, 0);
    lv_label_set_text(_lbl_wifi, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(_lbl_wifi, lv_color_hex(0x111111), 0);

    _bar_batt = lv_bar_create(_bar);
    lv_obj_set_size(_bar_batt, 40, 16);
    lv_obj_align(_bar_batt, LV_ALIGN_RIGHT_MID, -36, 0);
    lv_bar_set_range(_bar_batt, 0, 100);
    lv_bar_set_value(_bar_batt, 100, LV_ANIM_OFF);
    // AMOLED : fond barre = 0x0D0D0D
    lv_obj_set_style_bg_color(_bar_batt, lv_color_hex(0x0D0D0D), LV_PART_MAIN);
    lv_obj_set_style_bg_color(_bar_batt, lv_color_hex(0x4CAF50), LV_PART_INDICATOR);
    lv_obj_set_style_radius(_bar_batt, 3, 0);

    _lbl_batt = lv_label_create(_bar);
    lv_obj_align(_lbl_batt, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_obj_set_style_text_color(_lbl_batt, lv_color_white(), 0);
    lv_obj_set_style_text_font(_lbl_batt, &lv_font_montserrat_12, 0);
    lv_label_set_text(_lbl_batt, "?%");

    if (!os::kernel_time_is_valid()) {
        lv_obj_t* warn = lv_label_create(_bar);
        lv_obj_align(warn, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_color(warn, lv_color_hex(0xFF6B00), 0);
        lv_obj_set_style_text_font(warn, &lv_font_montserrat_12, 0);
        lv_label_set_text(warn, "⚠ Heure non définie");
    }
}

void status_bar_tick() {
    if (!_bar) return;

    char tbuf[16];
    if (os::kernel_time_is_valid()) {
        time_t now; time(&now);
        struct tm t; localtime_r(&now, &t);
        snprintf(tbuf, sizeof(tbuf), "%02d:%02d", t.tm_hour, t.tm_min);
    } else {
        snprintf(tbuf, sizeof(tbuf), "--:--");
    }
    lv_label_set_text(_lbl_time, tbuf);

    // BLE connecté = bleu vif, déconnecté = quasi-noir (pixels éteints)
    bool ble_conn = net::ble_is_connected();
    lv_obj_set_style_text_color(_lbl_ble,
        ble_conn ? lv_color_hex(0x2979FF) : lv_color_hex(0x111111), 0);

    // WiFi connecté = vert, déconnecté = quasi-noir
    bool wifi_conn = net::wifi_is_connected();
    lv_obj_set_style_text_color(_lbl_wifi,
        wifi_conn ? lv_color_hex(0x4CAF50) : lv_color_hex(0x111111), 0);

    int pct = hal::pmu_battery_percent();
    lv_bar_set_value(_bar_batt, pct, LV_ANIM_OFF);
    char pbuf[8];
    snprintf(pbuf, sizeof(pbuf), "%d%%", pct);
    lv_label_set_text(_lbl_batt, pbuf);
    lv_color_t col;
    if      (pct > 60) col = lv_color_hex(0x4CAF50);
    else if (pct > 20) col = lv_color_hex(0xFF9800);
    else               col = lv_color_hex(0xF44336);
    lv_obj_set_style_bg_color(_bar_batt, col, LV_PART_INDICATOR);
}

} // namespace ui
