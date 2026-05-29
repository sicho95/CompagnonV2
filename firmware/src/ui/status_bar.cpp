// ============================================================
// CompagnonV2 — ui/status_bar.cpp
// Status bar complète : heure, WiFi, BLE, batterie
// Mise à jour chaque seconde via ui_status_bar_tick().
// Créée sur lv_layer_top() — toujours au premier plan.
// ui_status_bar_raise() à appeler après chaque lv_scr_load().
// ============================================================
#include "status_bar.h"
#include "../hal/pmu.h"
#include <lvgl.h>
#include <Arduino.h>
#include <time.h>
#include <WiFi.h>

#define STATUS_BAR_H  28
#define ICON_FONT     (&lv_font_montserrat_14)
#define TEXT_FONT     (&lv_font_montserrat_14)

static lv_obj_t* _bar        = nullptr;
static lv_obj_t* _lbl_time   = nullptr;   // heure HH:MM
static lv_obj_t* _lbl_icons  = nullptr;   // WiFi + BLE + batterie (droite)

// ── Création ───────────────────────────────────────────────────────────────
void ui_status_bar_init() {
    if (!lv_display_get_default()) {
        Serial.println("[UI] status_bar init SKIP — aucun display LVGL");
        return;
    }

    _bar = lv_obj_create(lv_layer_top());
    lv_obj_set_size(_bar, LV_HOR_RES, STATUS_BAR_H);
    lv_obj_set_pos(_bar, 0, 0);
    lv_obj_set_style_bg_color(_bar, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_bg_opa(_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_bar, 1, 0);
    lv_obj_set_style_border_side(_bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(_bar, lv_color_hex(0x1A2A3A), 0);
    lv_obj_set_style_pad_hor(_bar, 8, 0);
    lv_obj_set_style_pad_ver(_bar, 0, 0);
    lv_obj_set_style_radius(_bar, 0, 0);
    lv_obj_clear_flag(_bar, LV_OBJ_FLAG_SCROLLABLE);

    // Label heure — aligné à gauche
    _lbl_time = lv_label_create(_bar);
    lv_label_set_text(_lbl_time, "--:--");
    lv_obj_set_style_text_font(_lbl_time, TEXT_FONT, 0);
    lv_obj_set_style_text_color(_lbl_time, lv_color_hex(0xCCCCDD), 0);
    lv_obj_align(_lbl_time, LV_ALIGN_LEFT_MID, 0, 0);

    // Label icônes — aligné à droite (WiFi BLE batterie)
    _lbl_icons = lv_label_create(_bar);
    lv_label_set_text(_lbl_icons, "- - -");
    lv_obj_set_style_text_font(_lbl_icons, ICON_FONT, 0);
    lv_obj_set_style_text_color(_lbl_icons, lv_color_hex(0x7EB8D4), 0);
    lv_obj_align(_lbl_icons, LV_ALIGN_RIGHT_MID, 0, 0);

    Serial.println("[UI] status_bar init OK");
}

void ui_status_bar_raise() {
    if (_bar) lv_obj_move_foreground(_bar);
}

// ── Mise à jour chaque seconde ─────────────────────────────────────────────
void ui_status_bar_tick() {
    if (!_bar) return;

    // Heure
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    char hhmm[8];
    if (t.tm_year > 70) {   // RTC valide
        snprintf(hhmm, sizeof(hhmm), "%02d:%02d", t.tm_hour, t.tm_min);
    } else {
        strncpy(hhmm, "--:--", sizeof(hhmm));
    }
    lv_label_set_text(_lbl_time, hhmm);

    // Icônes droite : WiFi  BLE  Batterie%
    char icons[32];
    bool wifi_ok = (WiFi.status() == WL_CONNECTED);
    bool ble_ok  = true;  // NimBLE actif dès le boot

    // Niveau batterie via PMU AXP2101
    int  batt_pct = (int)hal_pmu_get_battery_percent();
    char batt_chr = ' ';
    if      (batt_pct >= 80) batt_chr = LV_SYMBOL_BATTERY_FULL[0];   // fallback text
    else if (batt_pct >= 50) batt_chr = '3';
    else if (batt_pct >= 20) batt_chr = '2';
    else                     batt_chr = '1';

    snprintf(icons, sizeof(icons), "%s %s %d%%",
             wifi_ok ? LV_SYMBOL_WIFI    : LV_SYMBOL_CLOSE,
             ble_ok  ? LV_SYMBOL_BLUETOOTH : "-",
             batt_pct);
    lv_label_set_text(_lbl_icons, icons);

    lv_obj_align(_lbl_time,  LV_ALIGN_LEFT_MID,  0, 0);
    lv_obj_align(_lbl_icons, LV_ALIGN_RIGHT_MID, 0, 0);
}

void ui_power_menu_show() {
    Serial.println("[UI] power menu");
}
