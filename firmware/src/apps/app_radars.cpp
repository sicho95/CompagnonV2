#include "app_radars.h"
#include "../hal/display.h"
#include "../net/wifi_mgr.h"
#include "../ui/app_header.h"
#include "../ui/status_bar.h"
#include <lvgl.h>
#include <Arduino.h>

struct RadarNode {
    const char* id; const char* type; int rssi; float dist_km; bool online;
};
static const RadarNode _nodes[] = {
    { "!a3b2", "Meshtastic", -72, 1.2f, true  },
    { "!f91c", "LoRa GW",    -85, 3.6f, true  },
    { "!7d4e", "Météo node", -98, 8.1f, false },
};
static const int N_NODES = 3;
static lv_obj_t* _screen   = nullptr;
static lv_obj_t* _lbl_wifi = nullptr;

void AppRadars::init() {
    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_black(), 0);  // AMOLED
    lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, 0);

    lv_obj_t* hdr = lv_obj_create(_screen);
    lv_obj_set_size(hdr, ui_app_content_width(), ui_app_header_bar_h());
    lv_obj_set_pos(hdr, ui_app_content_x(), ui_app_header_bar_y());
    lv_obj_set_style_bg_color(hdr, lv_color_black(), 0);
    lv_obj_set_style_border_width(hdr, 1, 0);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(hdr, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_pad_all(hdr, 8, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(hdr);
    lv_label_set_text(title, LV_SYMBOL_WIFI "  Radars RF");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xCE93D8), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 8, 0);

    _lbl_wifi = lv_label_create(hdr);
    lv_label_set_text(_lbl_wifi, "");
    lv_obj_set_style_text_font(_lbl_wifi, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(_lbl_wifi, lv_color_hex(0x333333), 0);
    lv_obj_align(_lbl_wifi, LV_ALIGN_RIGHT_MID, -8, 0);

    lv_obj_t* list = lv_obj_create(_screen);
    lv_obj_set_size(list, ui_app_content_width(), ui_app_content_height());
    lv_obj_set_pos(list, ui_app_content_x(), ui_app_content_top());
    lv_obj_set_style_bg_color(list, lv_color_black(), 0);  // AMOLED
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_row(list, 0, 0);
    lv_obj_set_style_pad_all(list, 4, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);

    for (int i = 0; i < N_NODES; i++) {
        lv_obj_t* row = lv_obj_create(list);
        lv_obj_set_size(row, LV_PCT(100), 56);
        lv_obj_set_style_bg_color(row, lv_color_black(), 0);  // AMOLED
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(row, 0, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(0x1A1A1A), 0);
        lv_obj_set_style_pad_all(row, 10, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* dot = lv_obj_create(row);
        lv_obj_set_size(dot, 10, 10);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        // AMOLED : offline = 0x111111 (pixels quasi-éteints)
        lv_obj_set_style_bg_color(dot,
            _nodes[i].online ? lv_color_hex(0x4CAF50) : lv_color_hex(0x111111), 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_style_pad_right(dot, 10, 0);

        lv_obj_t* col = lv_obj_create(row);
        lv_obj_set_flex_grow(col, 1);
        lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(col, 0, 0);
        lv_obj_set_style_pad_all(col, 0, 0);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);

        char id_buf[32];
        snprintf(id_buf, sizeof(id_buf), "%s  •  %s", _nodes[i].id, _nodes[i].type);
        lv_obj_t* id_lbl = lv_label_create(col);
        lv_label_set_text(id_lbl, id_buf);
        lv_obj_set_style_text_font(id_lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(id_lbl, lv_color_white(), 0);

        char info_buf[32];
        snprintf(info_buf, sizeof(info_buf), "RSSI %d dBm  •  %.1f km",
                 _nodes[i].rssi, _nodes[i].dist_km);
        lv_obj_t* info = lv_label_create(col);
        lv_label_set_text(info, info_buf);
        lv_obj_set_style_text_font(info, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(info, lv_color_hex(0x444444), 0);
    }
    ui_app_header_attach(_screen);
    Serial.println("[Radars] init");
}

void AppRadars::onResume() {
    if (_lbl_wifi)
        lv_label_set_text(_lbl_wifi,
            WifiMgr::isConnected() ? LV_SYMBOL_WIFI " En ligne" : LV_SYMBOL_CLOSE " Hors ligne");
    lv_scr_load(_screen);
    ui_status_bar_raise();
    lv_obj_invalidate(lv_scr_act());
}
void AppRadars::update()  {}
void AppRadars::onPause() {}
