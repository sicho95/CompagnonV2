#include "app_header.h"
#include "../system/os_kernel.h"

namespace {

constexpr int HEADER_TOP_Y = 34;
constexpr int HEADER_BAR_Y = 36;
constexpr int HEADER_BAR_H = 44;
constexpr int HEADER_GAP   = 8;
constexpr int CLOSE_BTN_W  = 42;
constexpr int CLOSE_BTN_H  = 34;

static void _close_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    os::app_close_current();
}

} // namespace

void ui_app_header_attach(lv_obj_t* screen) {
    if (!screen) return;

    lv_obj_t* close_btn = lv_btn_create(screen);
    lv_obj_set_size(close_btn, CLOSE_BTN_W, CLOSE_BTN_H);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, -12, HEADER_TOP_Y);
    lv_obj_set_style_radius(close_btn, 14, 0);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0x101826), 0);
    lv_obj_set_style_bg_opa(close_btn, LV_OPA_85, 0);
    lv_obj_set_style_border_width(close_btn, 1, 0);
    lv_obj_set_style_border_color(close_btn, lv_color_hex(0x27344A), 0);
    lv_obj_set_style_shadow_width(close_btn, 0, 0);
    lv_obj_set_style_pad_all(close_btn, 0, 0);
    lv_obj_clear_flag(close_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(close_btn, _close_event_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(close_lbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(close_lbl, lv_color_hex(0xC7D1DE), 0);
    lv_obj_center(close_lbl);
}

int ui_app_header_bar_y() {
    return HEADER_BAR_Y;
}

int ui_app_header_bar_h() {
    return HEADER_BAR_H;
}

int ui_app_content_top() {
    return HEADER_BAR_Y + HEADER_BAR_H + HEADER_GAP;
}

int ui_app_content_height() {
    return LV_VER_RES - ui_app_content_top();
}
