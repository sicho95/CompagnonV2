#include "smarthome_app.h"
#include "../../config/nvs_config.h"
#include "../../hal/display.h"
#include "../../ui/app_header.h"
#include "../../ui/status_bar.h"
#include <Arduino.h>
#include <lvgl.h>

static lv_obj_t* _screen = nullptr;
static lv_obj_t* _status = nullptr;
static lv_obj_t* _detail = nullptr;

static bool _has_key(const char* key) {
    char tmp[8] = {};
    return nvs_get_api_key(key, tmp, sizeof(tmp));
}

static void _refresh() {
    if (!_status || !_detail) return;
    const bool id     = _has_key(NVS_KEY_TUYA_ID);
    const bool secret = _has_key(NVS_KEY_TUYA_SEC);
    const bool region = _has_key(NVS_KEY_TUYA_REGION);
    const bool user   = _has_key(NVS_KEY_TUYA_USER);

    if (id && secret) {
        lv_label_set_text(_status, "Tuya/SmartLife configure");
        lv_obj_set_style_text_color(_status, lv_color_hex(0x4FC3F7), 0);
    } else {
        lv_label_set_text(_status, "Configuration PWA requise");
        lv_obj_set_style_text_color(_status, lv_color_hex(0xFFB74D), 0);
    }

    char buf[180];
    snprintf(buf, sizeof(buf),
             "Access ID: %s\nAccess Secret: %s\nRegion: %s\nUser ID: %s\n\n"
             "Cette app attend le pont Tuya/Ecovacs apres provisioning BLE.",
             id ? "OK" : "manquant",
             secret ? "OK" : "manquant",
             region ? "OK" : "defaut",
             user ? "OK" : "facultatif");
    lv_label_set_text(_detail, buf);
}

void AppSmarthome::init() {
    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(_screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(_screen);
    lv_label_set_text(title, LV_SYMBOL_HOME "  Domotique");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x4FC3F7), 0);
    lv_obj_set_pos(title, ui_app_content_x() + 8, ui_app_header_bar_y() + 8);

    _status = lv_label_create(_screen);
    lv_obj_set_style_text_font(_status, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(_status, ui_app_content_x() + 10, ui_app_content_top() + 12);

    _detail = lv_label_create(_screen);
    lv_obj_set_width(_detail, ui_app_content_width() - 20);
    lv_label_set_long_mode(_detail, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(_detail, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(_detail, lv_color_hex(0xC8D2E0), 0);
    lv_obj_set_pos(_detail, ui_app_content_x() + 10, ui_app_content_top() + 58);

    _refresh();
    ui_app_header_attach(_screen);
    Serial.println("[SMARTHOME] init");
}

void AppSmarthome::update() {}

void AppSmarthome::onResume() {
    _refresh();
    lv_scr_load(_screen);
    ui_status_bar_raise();
    hal::display_request_refresh();
}

void AppSmarthome::onPause() {
    Serial.println("[SMARTHOME] pause");
}
