#include "app_bourse.h"
#include <lvgl.h>
#include <Arduino.h>

struct Ticker { const char* sym; float price; float chg_pct; };
static const Ticker _tickers[] = {
    { "BTC",   67000.0f, +2.4f },
    { "ETH",    3500.0f, +1.8f },
    { "AAPL",   189.5f, -0.3f },
    { "AMZN",   185.0f, +0.9f },
    { "NVDA",   875.0f, +3.1f },
};
static const int N_TICKERS = 5;
static lv_obj_t* _screen = nullptr;

static lv_color_t _chg_color(float pct) {
    if (pct > 0) return lv_color_hex(0x4CAF50);
    if (pct < 0) return lv_color_hex(0xF44336);
    return lv_color_hex(0x333333);
}

void AppBourse::init() {
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
    lv_label_set_text(title, LV_SYMBOL_CHARGE "  Bourse");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x4CAF50), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 8, 0);

    lv_obj_t* cont = lv_obj_create(_screen);
    lv_obj_set_size(cont, 480, LV_VER_RES - 36 - 44 - 8);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(cont, lv_color_black(), 0);  // AMOLED
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_row(cont, 6, 0);
    lv_obj_set_style_pad_column(cont, 6, 0);
    lv_obj_set_style_pad_all(cont, 6, 0);

    static lv_coord_t col_dsc[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static lv_coord_t row_dsc[] = { 64, 64, 64, LV_GRID_TEMPLATE_LAST };
    lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);

    for (int i = 0; i < N_TICKERS; i++) {
        lv_obj_t* card = lv_obj_create(cont);
        lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, i % 2, 1,
                                   LV_GRID_ALIGN_STRETCH, i / 2, 1);
        // AMOLED : 0x0A0A0A quasi-noir
        lv_obj_set_style_bg_color(card, lv_color_hex(0x0A0A0A), 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(card, 10, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x1A1A1A), 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_pad_all(card, 10, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* sym = lv_label_create(card);
        lv_label_set_text(sym, _tickers[i].sym);
        lv_obj_set_style_text_font(sym, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(sym, lv_color_white(), 0);
        lv_obj_align(sym, LV_ALIGN_TOP_LEFT, 0, 0);

        char price_buf[16];
        snprintf(price_buf, sizeof(price_buf), "%.2f", _tickers[i].price);
        lv_obj_t* price = lv_label_create(card);
        lv_label_set_text(price, price_buf);
        lv_obj_set_style_text_font(price, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(price, lv_color_white(), 0);
        lv_obj_align(price, LV_ALIGN_BOTTOM_LEFT, 0, 0);

        char chg_buf[16];
        snprintf(chg_buf, sizeof(chg_buf), "%+.1f%%", _tickers[i].chg_pct);
        lv_obj_t* chg = lv_label_create(card);
        lv_label_set_text(chg, chg_buf);
        lv_obj_set_style_text_font(chg, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(chg, _chg_color(_tickers[i].chg_pct), 0);
        lv_obj_align(chg, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    }
    Serial.println("[Bourse] init");
}

void AppBourse::onResume() {
    lv_scr_load_anim(_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}
void AppBourse::update()  {}
void AppBourse::onPause() {}
