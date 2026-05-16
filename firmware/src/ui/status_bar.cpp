#include "status_bar.h"
#include <lvgl.h>
#include <Arduino.h>

static lv_obj_t* _bar = nullptr;

void ui_status_bar_init() {
    _bar = lv_obj_create(lv_layer_top());
    lv_obj_set_size(_bar, LV_HOR_RES, 24);
    lv_obj_set_pos(_bar, 0, 0);
    lv_obj_set_style_bg_color(_bar, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(_bar, LV_OPA_50, 0);
    Serial.println("[UI] status_bar init OK");
}
void ui_status_bar_tick() {}
void ui_power_menu_show() {
    Serial.println("[UI] power menu");
    // TODO: afficher menu power LVGL
}
