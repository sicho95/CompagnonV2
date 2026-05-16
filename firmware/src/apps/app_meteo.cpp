#include "app_meteo.h"
#include <lvgl.h>
#include <Arduino.h>

static lv_obj_t* _screen = nullptr;

static const struct {
    const char* day; const char* icon; int hi; int lo;
} _forecast[] = {
    { "Auj.",  LV_SYMBOL_HOME,  22, 15 },
    { "Dem.",  LV_SYMBOL_DRIVE, 18, 12 },
    { "Lun.",  LV_SYMBOL_HOME,  24, 16 },
    { "Mar.",  LV_SYMBOL_DRIVE, 20, 13 },
};
static const int N_DAYS = 4;

void AppMeteo::init() {
    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_black(), 0);  // AMOLED
    lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, 0);

    lv_obj_t* hdr = lv_obj_create(_screen);
    lv_obj_set_size(hdr, 480, 44);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 36);
    lv_obj_set_style_bg_color(hdr, lv_color_black(), 0);
    lv_obj_set_style_border_width(hdr, 1, 0);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(hdr, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_pad_all(hdr, 8, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(hdr);
    lv_label_set_text(title, LV_SYMBOL_DRIVE "  Météo");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x64B5F6), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 8, 0);

    lv_obj_t* loc = lv_label_create(hdr);
    lv_label_set_text(loc, LV_SYMBOL_GPS " Le Plessis-Robinson");
    lv_obj_set_style_text_font(loc, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(loc, lv_color_hex(0x333333), 0);
    lv_obj_align(loc, LV_ALIGN_RIGHT_MID, -8, 0);

    // Carte principale — bleu nuit très sombre (~80% moins de pixels allumés vs bleu vif)
    lv_obj_t* main_card = lv_obj_create(_screen);
    lv_obj_set_size(main_card, 460, 130);
    lv_obj_align(main_card, LV_ALIGN_TOP_MID, 0, 88);
    lv_obj_set_style_bg_color(main_card, lv_color_hex(0x050D1A), 0);
    lv_obj_set_style_bg_opa(main_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(main_card, 14, 0);
    lv_obj_set_style_border_color(main_card, lv_color_hex(0x0A1828), 0);
    lv_obj_set_style_border_width(main_card, 1, 0);
    lv_obj_set_style_pad_all(main_card, 16, 0);
    lv_obj_clear_flag(main_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* temp_big = lv_label_create(main_card);
    lv_label_set_text(temp_big, "22°C");
    lv_obj_set_style_text_font(temp_big, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(temp_big, lv_color_white(), 0);
    lv_obj_align(temp_big, LV_ALIGN_LEFT_MID, 8, 0);

    lv_obj_t* desc = lv_label_create(main_card);
    lv_label_set_text(desc, "Partiellement nuageux\nHumidité : 62%\nVent : 12 km/h");
    lv_obj_set_style_text_font(desc, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(desc, lv_color_hex(0x7BAFD4), 0);
    lv_obj_align(desc, LV_ALIGN_RIGHT_MID, -8, 0);

    // Prévisions 4 jours
    lv_obj_t* fc_cont = lv_obj_create(_screen);
    lv_obj_set_size(fc_cont, 460, 80);
    lv_obj_align(fc_cont, LV_ALIGN_TOP_MID, 0, 228);
    lv_obj_set_style_bg_color(fc_cont, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(fc_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(fc_cont, 0, 0);
    lv_obj_set_style_pad_column(fc_cont, 6, 0);
    lv_obj_set_flex_flow(fc_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(fc_cont, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (int i = 0; i < N_DAYS; i++) {
        lv_obj_t* card = lv_obj_create(fc_cont);
        lv_obj_set_size(card, 104, 72);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x060606), 0);  // AMOLED quasi-noir
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(card, 10, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x1A1A1A), 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_pad_all(card, 6, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t* day = lv_label_create(card);
        lv_label_set_text(day, _forecast[i].day);
        lv_obj_set_style_text_font(day, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(day, lv_color_hex(0x555555), 0);

        lv_obj_t* ico = lv_label_create(card);
        lv_label_set_text(ico, _forecast[i].icon);
        lv_obj_set_style_text_font(ico, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(ico, lv_color_hex(0xFFD700), 0);

        char temp_buf[12];
        snprintf(temp_buf, sizeof(temp_buf), "%d° / %d°",
                 _forecast[i].hi, _forecast[i].lo);
        lv_obj_t* temps = lv_label_create(card);
        lv_label_set_text(temps, temp_buf);
        lv_obj_set_style_text_font(temps, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(temps, lv_color_white(), 0);
    }
    Serial.println("[Meteo] init");
}

void AppMeteo::onResume() {
    lv_scr_load_anim(_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}
void AppMeteo::update()  {}
void AppMeteo::onPause() {}
