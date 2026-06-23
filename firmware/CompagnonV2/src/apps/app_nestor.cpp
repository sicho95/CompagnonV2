#include "app_nestor.h"
#include "../hal/display.h"
#include "../ui/app_header.h"
#include "../ui/status_bar.h"
#include <Arduino.h>
#include <lvgl.h>

static lv_obj_t* _screen = nullptr;
static lv_obj_t* _last_intent = nullptr;

void AppNestor::init() {
    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(_screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(_screen);
    lv_label_set_text(title, LV_SYMBOL_AUDIO "  Nestor");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x4FC3F7), 0);
    lv_obj_set_pos(title, ui_app_content_x() + 8, ui_app_header_bar_y() + 8);

    lv_obj_t* body = lv_label_create(_screen);
    lv_label_set_text(body, "Assistant vocal pret.");
    lv_obj_set_style_text_font(body, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(body, lv_color_hex(0xCCCCDD), 0);
    lv_obj_align(body, LV_ALIGN_CENTER, 0, -12);

    _last_intent = lv_label_create(_screen);
    lv_label_set_text(_last_intent, "Aucune demande recue.");
    lv_obj_set_width(_last_intent, ui_app_content_width() - 16);
    lv_label_set_long_mode(_last_intent, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(_last_intent, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(_last_intent, lv_color_hex(0x777799), 0);
    lv_obj_set_pos(_last_intent, ui_app_content_x() + 8,
                   ui_app_content_top() + ui_app_content_height() - 54);

    ui_app_header_attach(_screen);
    Serial.println("[Nestor] init");
}

void AppNestor::update() {}

void AppNestor::onResume() {
    if (!_screen) return;
    lv_scr_load(_screen);
    ui_status_bar_raise();
    lv_obj_invalidate(lv_scr_act());
    Serial.printf("[Nestor] resume scr=%p active=%p\n", _screen, lv_scr_act());
}

void AppNestor::onPause() {}

void AppNestor::handleIntent(const char* intent, const char* param) {
    if (!_last_intent) return;
    char buf[180];
    snprintf(buf, sizeof(buf), "%s: %s",
             intent ? intent : "intent",
             param ? param : "");
    lv_label_set_text(_last_intent, buf);
}
