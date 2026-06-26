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
#include "../hal/display.h"
#include "../ui/app_header.h"
#include "../ui/status_bar.h"
#include "../ui/ui_dispatch.h"
#include <lvgl.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <memory>

#define METEO_INSEE  "92050"
#define METEO_URL    "https://api.meteo-concept.com/api/forecast/daily?token=%s&insee=" METEO_INSEE
#define FORECAST_DAYS 4
#define METEO_HTTP_TIMEOUT_MS 5000

static lv_obj_t* _screen      = nullptr;
static lv_obj_t* _temp_big    = nullptr;
static lv_obj_t* _desc_lbl    = nullptr;
static lv_obj_t* _day_lbl[FORECAST_DAYS]  = {};
static lv_obj_t* _tmin_lbl[FORECAST_DAYS] = {};
static lv_obj_t* _tmax_lbl[FORECAST_DAYS] = {};
static uint32_t  _fetch_gen   = 0;
static bool      _fetch_in_flight = false;

static const char* _weather_label(int code);

namespace {

struct MeteoFetchCtx {
    uint32_t gen;
};

struct MeteoFetchResult {
    uint32_t gen = 0;
    bool ok = false;
    char temp[40] = "--°C";
    char desc[96] = "Chargement...";
    char hi[FORECAST_DAYS][12] = {};
    char lo[FORECAST_DAYS][12] = {};
};

static void _apply_fetch_result(MeteoFetchResult* result) {
    if (!result) return;
    std::unique_ptr<MeteoFetchResult> holder(result);
    _fetch_in_flight = false;
    if (holder->gen != _fetch_gen) return;
    if (!_temp_big || !_desc_lbl) return;

    lv_label_set_text(_temp_big, holder->temp);
    lv_label_set_text(_desc_lbl, holder->desc);
    for (int i = 0; i < FORECAST_DAYS; ++i) {
        if (_tmax_lbl[i] && holder->hi[i][0]) lv_label_set_text(_tmax_lbl[i], holder->hi[i]);
        if (_tmin_lbl[i] && holder->lo[i][0]) lv_label_set_text(_tmin_lbl[i], holder->lo[i]);
    }
    if (_screen == lv_scr_act()) {
        lv_obj_invalidate(lv_scr_act());
    }
}

static void _meteo_fetch_task(void* user_data) {
    std::unique_ptr<MeteoFetchCtx> ctx(static_cast<MeteoFetchCtx*>(user_data));
    if (!ctx) {
        vTaskDelete(nullptr);
        return;
    }

    auto* result = new MeteoFetchResult();
    result->gen = ctx->gen;

    if (WiFi.status() != WL_CONNECTED) {
        strlcpy(result->desc, "Pas de WiFi", sizeof(result->desc));
        ui::dispatch_post([result]() { _apply_fetch_result(result); });
        vTaskDelete(nullptr);
        return;
    }

    char token[64] = {};
    if (!nvs_get_api_key(NVS_KEY_METEO, token, sizeof(token))) {
        strlcpy(result->desc, "Clé manquante", sizeof(result->desc));
        ui::dispatch_post([result]() { _apply_fetch_result(result); });
        Serial.println("[Meteo] NVS_KEY_METEO non defini");
        vTaskDelete(nullptr);
        return;
    }

    char url[200];
    snprintf(url, sizeof(url), METEO_URL, token);

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, url);
    http.setTimeout(METEO_HTTP_TIMEOUT_MS);
    http.addHeader("Accept", "application/json");
    int code = http.GET();
    if (code != 200) {
        snprintf(result->desc, sizeof(result->desc), "HTTP %d", code);
        Serial.printf("[Meteo] HTTP %d\n", code);
        http.end();
        ui::dispatch_post([result]() { _apply_fetch_result(result); });
        vTaskDelete(nullptr);
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();
    if (err) {
        strlcpy(result->desc, "Erreur JSON", sizeof(result->desc));
        Serial.printf("[Meteo] JSON err: %s\n", err.c_str());
        ui::dispatch_post([result]() { _apply_fetch_result(result); });
        vTaskDelete(nullptr);
        return;
    }

    JsonArray forecast = doc["forecast"];
    if (forecast.isNull() || forecast.size() == 0) {
        strlcpy(result->desc, "Données vides", sizeof(result->desc));
        ui::dispatch_post([result]() { _apply_fetch_result(result); });
        vTaskDelete(nullptr);
        return;
    }

    JsonObject d0  = forecast[0];
    int tmin0      = d0["tmin"] | 0;
    int tmax0      = d0["tmax"] | 0;
    int weather0   = d0["weather"] | 0;
    snprintf(result->temp, sizeof(result->temp), "%d°C", tmax0);
    snprintf(result->desc, sizeof(result->desc), "%s\nMin %d° / Max %d°",
             _weather_label(weather0), tmin0, tmax0);

    for (int i = 0; i < FORECAST_DAYS && i < (int)forecast.size(); i++) {
        JsonObject d = forecast[i];
        int tmin = d["tmin"] | 0;
        int tmax = d["tmax"] | 0;
        snprintf(result->hi[i], sizeof(result->hi[i]), "%d°", tmax);
        snprintf(result->lo[i], sizeof(result->lo[i]), "%d°", tmin);
    }
    result->ok = true;
    Serial.println("[Meteo] donnees Meteo-Concept OK");
    ui::dispatch_post([result]() { _apply_fetch_result(result); });
    vTaskDelete(nullptr);
}

} // namespace

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
    lv_obj_set_size(main_card, ui_app_content_width(), 130);
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
    lv_obj_set_size(fc_cont, ui_app_content_width(), 80);
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
    ui_app_header_attach(_screen);
    Serial.println("[Meteo] init");
}

void AppMeteo::onResume() {
    lv_scr_load(_screen);
    ui_status_bar_raise();
    lv_obj_invalidate(lv_scr_act());
    if (_desc_lbl) lv_label_set_text(_desc_lbl, "Chargement...");
    if (_temp_big) lv_label_set_text(_temp_big, "--°C");
    _fetch();
}

void AppMeteo::update() {}
void AppMeteo::onPause() {
    ++_fetch_gen;
}

void AppMeteo::_fetch() {
    if (_fetch_in_flight) {
        Serial.println("[Meteo] fetch ignored, already in flight");
        return;
    }
    ++_fetch_gen;
    _fetch_in_flight = true;
    auto* ctx = new MeteoFetchCtx{_fetch_gen};
    BaseType_t ok = xTaskCreatePinnedToCore(_meteo_fetch_task, "meteo_fetch",
                                            8192, ctx, 1, nullptr, 0);
    if (ok != pdPASS) {
        _fetch_in_flight = false;
        delete ctx;
        if (_desc_lbl) lv_label_set_text(_desc_lbl, "Erreur tâche");
        Serial.println("[Meteo] fetch task create FAILED");
    }
}
