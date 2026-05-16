#include "app_nestor.h"
#include "../net/http_client.h"
#include "../voice/voice_engine.h"
#include <lvgl.h>
#include <Arduino.h>

static lv_obj_t* _screen     = nullptr;
static lv_obj_t* _msg_list   = nullptr;
static lv_obj_t* _lbl_status = nullptr;

static void _add_bubble(const char* text, bool is_user) {
    if (!_msg_list) return;
    lv_obj_t* bubble = lv_obj_create(_msg_list);
    lv_obj_set_width(bubble, LV_PCT(85));
    lv_obj_set_height(bubble, LV_SIZE_CONTENT);
    // AMOLED : bulles très sombres
    lv_obj_set_style_bg_color(bubble,
        is_user ? lv_color_hex(0x0A2A4A) : lv_color_hex(0x0F0F0F), 0);
    lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bubble, 12, 0);
    lv_obj_set_style_border_width(bubble, 0, 0);
    lv_obj_set_style_pad_all(bubble, 10, 0);
    if (is_user)
        lv_obj_align(bubble, LV_ALIGN_RIGHT_MID, -4, 0);
    else
        lv_obj_align(bubble, LV_ALIGN_LEFT_MID, 4, 0);

    lv_obj_t* lbl = lv_label_create(bubble);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, LV_PCT(100));
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_scroll_to_view(bubble, LV_ANIM_ON);
}

void AppNestor::init() {
    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_black(), 0);  // AMOLED
    lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, 0);

    lv_obj_t* header = lv_obj_create(_screen);
    lv_obj_set_size(header, 480, 44);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 36);
    lv_obj_set_style_bg_color(header, lv_color_black(), 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(header, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_pad_all(header, 8, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, LV_SYMBOL_AUDIO "  Nestor — Agent IA");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x64B5F6), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 8, 0);

    _lbl_status = lv_label_create(header);
    lv_label_set_text(_lbl_status, "");
    lv_obj_set_style_text_font(_lbl_status, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(_lbl_status, lv_color_hex(0x4CAF50), 0);
    lv_obj_align(_lbl_status, LV_ALIGN_RIGHT_MID, -8, 0);

    _msg_list = lv_obj_create(_screen);
    lv_obj_set_size(_msg_list, 480, LV_VER_RES - 36 - 44 - 48);
    lv_obj_align(_msg_list, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_style_bg_color(_msg_list, lv_color_black(), 0);  // AMOLED
    lv_obj_set_style_bg_opa(_msg_list, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_msg_list, 0, 0);
    lv_obj_set_style_pad_row(_msg_list, 8, 0);
    lv_obj_set_style_pad_all(_msg_list, 6, 0);
    lv_obj_set_flex_flow(_msg_list, LV_FLEX_FLOW_COLUMN);

    lv_obj_t* footer = lv_obj_create(_screen);
    lv_obj_set_size(footer, 480, 44);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(footer, lv_color_black(), 0);
    lv_obj_set_style_border_width(footer, 1, 0);
    lv_obj_set_style_border_side(footer, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(footer, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_pad_all(footer, 8, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* hint = lv_label_create(footer);
    lv_label_set_text(hint, LV_SYMBOL_AUDIO " Parlez pour interroger Nestor");
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x252525), 0);
    lv_obj_align(hint, LV_ALIGN_CENTER, 0, 0);

    Serial.println("[Nestor] init");
}

void AppNestor::onResume() {
    lv_label_set_text(_lbl_status, LV_SYMBOL_AUDIO " Écoute");
    lv_scr_load_anim(_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}
void AppNestor::update() {}
void AppNestor::onPause() {
    if (_lbl_status) lv_label_set_text(_lbl_status, "");
}

void AppNestor::handleIntent(const char* intent, const char* param) {
    if (strcmp(intent, "query") == 0 || strcmp(intent, "free_speech") == 0) {
        _add_bubble(param, true);
        if (_lbl_status) lv_label_set_text(_lbl_status, "⌛ Réflexion...");
        String reply = HttpClient::chatCompletion(String(param));
        if (reply.length() > 0) {
            _add_bubble(reply.c_str(), false);
            voice::speak(reply.c_str());
        } else {
            const char* err = "Je n'ai pas pu obtenir de réponse.";
            _add_bubble(err, false);
            voice::speak(err);
        }
        if (_lbl_status) lv_label_set_text(_lbl_status, LV_SYMBOL_AUDIO " Écoute");
    }
}
