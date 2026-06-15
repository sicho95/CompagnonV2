// ============================================================
// CompagnonV2 — apps/app_meteo.cpp
// API : Météo-Concept (https://api.meteo-concept.com/)
// Clé NVS  : NVS_KEY_METEO = "meteo_key" (token Bearer)
// Endpoint : GET /api/forecast/daily?token=<KEY>&insee=92050
//            (INSEE 92050 = Le Plessis-Robinson)
// Réponse JSON : { "city":{...}, "forecast":[{"tmin":x,"tmax":y,"weather":w,...},...] }
// ============================================================
#include "app_meteo.h"
#include "../config/nvs_config.h"
#include "../ui/app_header.h"
#include "../ui/status_bar.h"
#include <lvgl.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#define METEO_INSEE  "92050"
#define METEO_URL    "https://api.meteo-concept.com/api/forecast/daily?token=%s&insee=" METEO_INSEE
#define FORECAST_DAYS 4

static lv_obj_t* _screen      = nullptr;
static lv_obj_t* _temp_big    = nullptr;
static lv_obj_t* _desc_lbl    = nullptr;
static lv_obj_t* _day_lbl[FORECAST_DAYS]  = {};
static lv_obj_t* _tmin_lbl[FORECAST_DAYS] = {};
static lv_obj_t* _tmax_lbl[FORECAST_DAYS] = {};

// Codes weather Météo-Concept → libellé court (UTF-8 natif)
static const char* _weather_label(int code) {
    if (code <= 2)  return "Ensoleillé";
    if (code <= 5)  return "Peu nuageux";
    if (code <= 10) return "Nuageux";
    if (code <= 16) return "Couvert";
    if (code <= 22) return "Pluvieux";
    if (code <= 28) return "Orageux";
    if (code <= 34) return "Neigeux";
    return "Variable";
}

static const char* _weather_icon(int code) {
    if (code <= 2)  return LV_SYMBOL_CHARGE;
    if (code <= 10) return LV_SYMBOL_HOME;
    if (code <= 22) return LV_SYMBOL_DRIVE;
    if (code <= 28) return LV_SYMBOL_WARNING;
    return LV_SYMBOL_STOP;
}

void AppMeteo::init() {
    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, 0);
    ui_app_header_attach(_screen);

    lv_obj_t* hdr = lv_obj_create(_screen);
    lv_obj_set_size(hdr, 480, 44);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, ui_app_header_bar_y());
    lv_obj_set_style_bg_color(hdr, lv_color_black(), 0);
    lv_obj_set_style_border_width(hdr, 1, 0);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(hdr, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_pad_all(hdr, 8, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(hdr);
    // LV_SYMBOL suivi d'un literal UTF-8 — pas d'échappement hex
    lv_label_set_text(title, LV_SYMBOL_DRIVE "  Météo");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x64B5F6), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 8, 0);

    lv_obj_t* src = lv_label_create(hdr);
    lv_label_set_text(src, "Météo-Concept");
    lv_obj_set_style_text_font(src, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(src, lv_color_hex(0x555555), 0);
    lv_obj_align(src, LV_ALIGN_RIGHT_MID, -8, 0);

    lv_obj_t* main_card = lv_obj_create(_screen);
    lv_obj_set_size(main_card, 460, 130);
    lv_obj_align(main_card, LV_ALIGN_TOP_MID, 0, ui_app_content_top());
    lv_obj_set_style_bg_color(main_card, lv_color_hex(0x050D1A), 0);
    lv_obj_set_style_bg_opa(main_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(main_card, 14, 0);
    lv_obj_set_style_border_color(main_card, lv_color_hex(0x0A1828), 0);
    lv_obj_set_style_border_width(main_card, 1, 0);
    lv_obj_set_style_pad_all(main_card, 16, 0);
    lv_obj_clear_flag(main_card, LV_OBJ_FLAG_SCROLLABLE);

    _temp_big = lv_label_create(main_card);
    lv_label_set_text(_temp_big, "--°C");
    lv_obj_set_style_text_font(_temp_big, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(_temp_big, lv_color_white(), 0);
    lv_obj_align(_temp_big, LV_ALIGN_LEFT_MID, 8, 0);

    _desc_lbl = lv_label_create(main_card);
    lv_label_set_text(_desc_lbl, "Chargement...");
    lv_obj_set_style_text_font(_desc_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(_desc_lbl, lv_color_hex(0x7BAFD4), 0);
    lv_obj_align(_desc_lbl, LV_ALIGN_RIGHT_MID, -8, 0);

    lv_obj_t* fc_cont = lv_obj_create(_screen);
    lv_obj_set_size(fc_cont, 460, 80);
    lv_obj_align(fc_cont, LV_ALIGN_TOP_MID, 0, ui_app_content_top() + 140);
    lv_obj_set_style_bg_color(fc_cont, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(fc_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(fc_cont, 0, 0);
    lv_obj_set_style_pad_column(fc_cont, 6, 0);
    lv_obj_set_flex_flow(fc_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(fc_cont, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    static const char* day_names[] = { "Auj.", "Dem.", "J+2", "J+3" };
    for (int i = 0; i < FORECAST_DAYS; i++) {
        lv_obj_t* card = lv_obj_create(fc_cont);
        lv_obj_set_size(card, 104, 72);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x060606), 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(card, 10, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x1A1A1A), 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_pad_all(card, 6, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        _day_lbl[i] = lv_label_create(card);
        lv_label_set_text(_day_lbl[i], day_names[i]);
        lv_obj_set_style_text_font(_day_lbl[i], &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(_day_lbl[i], lv_color_hex(0x555555), 0);

        _tmax_lbl[i] = lv_label_create(card);
        lv_label_set_text(_tmax_lbl[i], "--°");
        lv_obj_set_style_text_font(_tmax_lbl[i], &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(_tmax_lbl[i], lv_color_hex(0xFF9800), 0);

        _tmin_lbl[i] = lv_label_create(card);
        lv_label_set_text(_tmin_lbl[i], "--°");
        lv_obj_set_style_text_font(_tmin_lbl[i], &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(_tmin_lbl[i], lv_color_hex(0x64B5F6), 0);
    }
    Serial.println("[Meteo] init");
}

void AppMeteo::onResume() {
    lv_scr_load(_screen);
    ui_status_bar_raise();
    lv_obj_invalidate(lv_scr_act());
    _fetch();
}

void AppMeteo::update() {}
void AppMeteo::onPause() {}

void AppMeteo::_fetch() {
    if (WiFi.status() != WL_CONNECTED) {
        lv_label_set_text(_desc_lbl, "Pas de WiFi");
        return;
    }
    char token[64] = {};
    if (!nvs_get_api_key(NVS_KEY_METEO, token, sizeof(token))) {
        lv_label_set_text(_desc_lbl, "Clé manquante");
        Serial.println("[Meteo] NVS_KEY_METEO non defini");
        return;
    }
    char url[200];
    snprintf(url, sizeof(url), METEO_URL, token);

    HTTPClient http;
    http.begin(url);
    http.addHeader("Accept", "application/json");
    int code = http.GET();
    if (code != 200) {
        Serial.printf("[Meteo] HTTP %d\n", code);
        lv_label_set_text(_desc_lbl, "Erreur réseau");
        http.end();
        return;
    }

    // ArduinoJson v7 : JsonDocument (StaticJsonDocument déprécié)
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();
    if (err) {
        Serial.printf("[Meteo] JSON err: %s\n", err.c_str());
        lv_label_set_text(_desc_lbl, "Erreur JSON");
        return;
    }

    JsonArray forecast = doc["forecast"];
    if (forecast.isNull() || forecast.size() == 0) {
        lv_label_set_text(_desc_lbl, "Données vides");
        return;
    }

    JsonObject d0  = forecast[0];
    int tmin0      = d0["tmin"] | 0;
    int tmax0      = d0["tmax"] | 0;
    int weather0   = d0["weather"] | 0;

    // Utiliser des tableaux assez grands pour les caractères UTF-8 (é = 2 octets)
    char buf[40];
    snprintf(buf, sizeof(buf), "%d°C", tmax0);
    lv_label_set_text(_temp_big, buf);

    char desc[80];
    snprintf(desc, sizeof(desc), "%s\nMin %d° / Max %d°",
             _weather_label(weather0), tmin0, tmax0);
    lv_label_set_text(_desc_lbl, desc);

    for (int i = 0; i < FORECAST_DAYS && i < (int)forecast.size(); i++) {
        JsonObject d  = forecast[i];
        int tmin = d["tmin"] | 0;
        int tmax = d["tmax"] | 0;
        char hi[12], lo[12];
        snprintf(hi, sizeof(hi), "%d°", tmax);
        snprintf(lo, sizeof(lo), "%d°", tmin);
        lv_label_set_text(_tmax_lbl[i], hi);
        lv_label_set_text(_tmin_lbl[i], lo);
    }
    Serial.println("[Meteo] donnees Meteo-Concept OK");
}
