#include "app_rappels.h"
#include "../hal/display.h"
#include "../ui/app_header.h"
#include "../ui/status_bar.h"
#include <Arduino.h>
#include <lvgl.h>
#include <string.h>

static lv_obj_t* _screen = nullptr;
static lv_obj_t* _list = nullptr;

static void _format_time(time_t t, char* out, size_t out_len) {
    if (t <= 0) {
        strlcpy(out, "--:--", out_len);
        return;
    }
    struct tm tmv;
    localtime_r(&t, &tmv);
    snprintf(out, out_len, "%02d/%02d %02d:%02d",
             tmv.tm_mday, tmv.tm_mon + 1, tmv.tm_hour, tmv.tm_min);
}

static void _refresh_list() {
    if (!_list) return;
    lv_obj_clean(_list);

    const auto& reminders = ReminderStore::getAll();
    if (reminders.empty()) {
        lv_obj_t* empty = lv_label_create(_list);
        lv_label_set_text(empty, "Aucun rappel.");
        lv_obj_set_style_text_font(empty, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(empty, lv_color_hex(0x777799), 0);
        return;
    }

    for (const auto& r : reminders) {
        lv_obj_t* row = lv_obj_create(_list);
        lv_obj_set_size(row, LV_PCT(100), 58);
        lv_obj_set_style_bg_color(row, lv_color_hex(0x0A0A12), 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(0x202030), 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_set_style_pad_all(row, 8, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* title = lv_label_create(row);
        lv_label_set_text(title, r.label.c_str());
        lv_obj_set_width(title, LV_PCT(70));
        lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(title, lv_color_white(), 0);
        lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

        char when[24];
        _format_time(r.datetime, when, sizeof(when));
        lv_obj_t* date = lv_label_create(row);
        lv_label_set_text(date, when);
        lv_obj_set_style_text_font(date, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(date,
            r.enabled ? lv_color_hex(0x4FC3F7) : lv_color_hex(0x666666), 0);
        lv_obj_align(date, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    }
}

void AppRappels::init() {
    ReminderStore::load();

    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(_screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(_screen);
    lv_label_set_text(title, LV_SYMBOL_BELL "  Rappels");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFD54F), 0);
    lv_obj_set_pos(title, ui_app_content_x() + 8, ui_app_header_bar_y() + 8);

    _list = lv_obj_create(_screen);
    lv_obj_set_pos(_list, ui_app_content_x(), ui_app_content_top() - 4);
    lv_obj_set_size(_list, ui_app_content_width(), ui_app_content_height() - 4);
    lv_obj_set_style_bg_opa(_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_list, 0, 0);
    lv_obj_set_style_pad_all(_list, 0, 0);
    lv_obj_set_style_pad_row(_list, 8, 0);
    lv_obj_set_flex_flow(_list, LV_FLEX_FLOW_COLUMN);

    _refresh_list();
    ui_app_header_attach(_screen);
    Serial.println("[Rappels] init");
}

void AppRappels::update() {}

void AppRappels::onResume() {
    ReminderStore::load();
    _refresh_list();
    lv_scr_load(_screen);
    ui_status_bar_raise();
    lv_obj_invalidate(lv_scr_act());
    hal::display_force_refresh();
    Serial.printf("[Rappels] resume scr=%p active=%p\n", _screen, lv_scr_act());
}

void AppRappels::onPause() {
    Serial.printf("[Rappels] pause scr=%p active=%p\n", _screen, lv_scr_act());
}

void AppRappels::handleIntent(const char* intent, const char* param) {
    if (!intent || strcmp(intent, "create_reminder") != 0 || !param || !param[0]) return;
    Reminder r;
    r.id = 0;
    r.label = param;
    r.datetime = time(nullptr) + 3600;
    r.advance_minutes = 0;
    r.enabled = true;
    ReminderStore::add(r);
    _refresh_list();
}

time_t AppRappels::nextEpoch() const {
    time_t now = time(nullptr);
    time_t best = 0;
    for (const auto& r : ReminderStore::getAll()) {
        if (!r.enabled) continue;
        time_t trigger = r.datetime - (time_t)(r.advance_minutes * 60);
        if (trigger <= now) continue;
        if (best == 0 || trigger < best) best = trigger;
    }
    return best;
}
