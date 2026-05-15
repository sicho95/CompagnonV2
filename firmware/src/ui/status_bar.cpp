// =============================================================
// CompagnonV2 — ui/status_bar.cpp
// Barre de statut LVGL 9
//
// Safe area :
//   x=BORDER_H(20), y=BORDER_V(10), w=440, h=36
//
// Widgets :
//   lbl_datetime  : label date/heure FR  (gauche)
//   lbl_ble       : label icône BLE \uF294 (centre-gauche)
//   lbl_wifi      : label icône WiFi \uF1EB (centre-droite)
//   bar_battery   : lv_bar % coloré     (droite)
//   lbl_batt_pct  : label % sur la jauge
//
// Couleurs jauge :
//   > 30% → vert    (0x22AA44)
//   15-30% → orange (0xFF8800)
//   < 15% → rouge   (0xCC2200)
// =============================================================
#include "status_bar.h"
#include "../config/ui_config.h"
#include <time.h>
#include <Arduino.h>

// Globals définis dans main.cpp
extern volatile bool    g_wifi_connected;
extern volatile bool    g_ble_connected;
extern volatile uint8_t g_battery_pct;
extern volatile bool    g_charging;

// ─── Widgets ─────────────────────────────────────────────────
static lv_obj_t  *bar_root     = nullptr;  // conteneur
static lv_obj_t  *lbl_datetime = nullptr;
static lv_obj_t  *lbl_ble      = nullptr;
static lv_obj_t  *lbl_wifi     = nullptr;
static lv_obj_t  *bar_battery  = nullptr;
static lv_obj_t  *lbl_batt_pct = nullptr;

// ─── Styles ──────────────────────────────────────────────────
static lv_style_t style_bar_root;
static lv_style_t style_batt_ind;

// ─── Helpers ─────────────────────────────────────────────────
static lv_color_t battery_color(uint8_t pct) {
    if (pct > 30) return lv_color_hex(0x22AA44);
    if (pct > 15) return lv_color_hex(0xFF8800);
    return lv_color_hex(0xCC2200);
}

static void update_battery() {
    if (!bar_battery || !lbl_batt_pct) return;

    // Met à jour la valeur de la barre
    lv_bar_set_value(bar_battery, g_battery_pct, LV_ANIM_OFF);

    // Change la couleur de l'indicateur selon le niveau
    lv_obj_set_style_bg_color(bar_battery, battery_color(g_battery_pct), LV_PART_INDICATOR);

    // Texte % avec ⚡ si en charge
    char buf[16];
    if (g_charging) {
        snprintf(buf, sizeof(buf), "\u26A1%d%%", g_battery_pct);
    } else {
        snprintf(buf, sizeof(buf), "%d%%", g_battery_pct);
    }
    lv_label_set_text(lbl_batt_pct, buf);
}

static void update_datetime() {
    if (!lbl_datetime) return;
    // Utilise l'heure système (settimeofday après NTP/RTC sync)
    // TZ doit être défini : setenv("TZ","CET-1CEST,M3.5.0,M10.5.0/3",1); tzset();
    time_t now = time(nullptr);
    struct tm *t = localtime(&now);
    if (!t) {
        lv_label_set_text(lbl_datetime, "--/-- --:--");
        return;
    }
    // Format FR : "15 mai 2026 - 17:42"
    static const char *mois[] = {
        "jan","fev","mar","avr","mai","jun",
        "jul","aou","sep","oct","nov","dec"
    };
    char buf[32];
    snprintf(buf, sizeof(buf), "%d %s %d - %02d:%02d",
        t->tm_mday, mois[t->tm_mon],
        t->tm_year + 1900,
        t->tm_hour, t->tm_min);
    lv_label_set_text(lbl_datetime, buf);
}

// ─── Init ─────────────────────────────────────────────────────
void status_bar_init() {
    lv_obj_t *scr = lv_screen_active();

    // ── Conteneur fond semi-transparent ──────────────────────
    lv_style_init(&style_bar_root);
    lv_style_set_bg_color(&style_bar_root, lv_color_hex(0x111111));
    lv_style_set_bg_opa(&style_bar_root, LV_OPA_80);
    lv_style_set_border_width(&style_bar_root, 0);
    lv_style_set_pad_all(&style_bar_root, 0);
    lv_style_set_radius(&style_bar_root, 0);

    bar_root = lv_obj_create(scr);
    lv_obj_add_style(bar_root, &style_bar_root, 0);
    lv_obj_set_pos(bar_root,  STATUS_BAR_X, STATUS_BAR_Y);
    lv_obj_set_size(bar_root, STATUS_BAR_W, STATUS_BAR_H);
    lv_obj_clear_flag(bar_root, LV_OBJ_FLAG_SCROLLABLE);

    // ── Date/heure (gauche) ───────────────────────────────────
    lbl_datetime = lv_label_create(bar_root);
    lv_label_set_text(lbl_datetime, "-- --- ---- - --:--");
    lv_obj_set_style_text_font(lbl_datetime, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_datetime, lv_color_white(), 0);
    lv_obj_align(lbl_datetime, LV_ALIGN_LEFT_MID, 6, 0);

    // ── Icône BLE (LV_SYMBOL_BLUETOOTH si dispo, sinon texte) ─
    lbl_ble = lv_label_create(bar_root);
    lv_label_set_text(lbl_ble, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_font(lbl_ble, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_ble, lv_color_hex(0x4488FF), 0);
    lv_obj_set_style_opa(lbl_ble, LV_OPA_TRANSP, 0);  // caché par défaut
    lv_obj_align(lbl_ble, LV_ALIGN_RIGHT_MID, -80, 0);

    // ── Icône WiFi ────────────────────────────────────────────
    lbl_wifi = lv_label_create(bar_root);
    lv_label_set_text(lbl_wifi, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(lbl_wifi, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_wifi, lv_color_hex(0x44DDAA), 0);
    lv_obj_set_style_opa(lbl_wifi, LV_OPA_TRANSP, 0);  // caché par défaut
    lv_obj_align(lbl_wifi, LV_ALIGN_RIGHT_MID, -56, 0);

    // ── Jauge batterie (droite) ───────────────────────────────
    lv_style_init(&style_batt_ind);
    lv_style_set_bg_color(&style_batt_ind, lv_color_hex(0x22AA44));
    lv_style_set_radius(&style_batt_ind, 2);

    bar_battery = lv_bar_create(bar_root);
    lv_bar_set_range(bar_battery, 0, 100);
    lv_bar_set_value(bar_battery, 100, LV_ANIM_OFF);
    lv_obj_set_size(bar_battery, 38, 16);
    lv_obj_align(bar_battery, LV_ALIGN_RIGHT_MID, -4, 0);
    // Fond de la jauge
    lv_obj_set_style_bg_color(bar_battery, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_radius(bar_battery, 3, LV_PART_MAIN);
    lv_obj_set_style_border_color(bar_battery, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_set_style_border_width(bar_battery, 1, LV_PART_MAIN);
    // Indicateur (couleur dynamique)
    lv_obj_add_style(bar_battery, &style_batt_ind, LV_PART_INDICATOR);

    // ── Label % (centré sur la jauge) ─────────────────────────
    lbl_batt_pct = lv_label_create(bar_root);
    lv_label_set_text(lbl_batt_pct, "100%");
    lv_obj_set_style_text_font(lbl_batt_pct, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_batt_pct, lv_color_white(), 0);
    lv_obj_align_to(lbl_batt_pct, bar_battery, LV_ALIGN_CENTER, 0, 0);

    // Premier rendu
    update_datetime();
    update_battery();
}

// ─── Tick (à appeler ~1/s depuis task_os_main) ────────────────
void status_bar_tick() {
    update_datetime();
    update_battery();

    // Icône BLE
    if (g_ble_connected) {
        lv_obj_set_style_opa(lbl_ble, LV_OPA_COVER, 0);
    } else {
        lv_obj_set_style_opa(lbl_ble, LV_OPA_TRANSP, 0);
    }

    // Icône WiFi
    if (g_wifi_connected) {
        lv_obj_set_style_opa(lbl_wifi, LV_OPA_COVER, 0);
    } else {
        lv_obj_set_style_opa(lbl_wifi, LV_OPA_TRANSP, 0);
    }
}
