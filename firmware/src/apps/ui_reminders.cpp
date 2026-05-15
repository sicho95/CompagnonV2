// ============================================================
// CompagnonV2 — App Rappels (LVGL 8.x)
// ============================================================
#include "ui_reminders.h"
#include "../voice/voice_engine.h"
#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <time.h>
#include <vector>
#include <string>
#include <algorithm>

namespace apps {
namespace reminders {

struct Reminder {
    char id[16];
    char title[64];
    char description[128];
    char datetime[32];   // "2026-05-16T09:00:00"
    int  advance_min;
    bool done;
};

static std::vector<Reminder> _reminders;
static const char* STORAGE_PATH = "/spiffs/reminders.json";

static lv_obj_t* _screen     = nullptr;
static lv_obj_t* _list       = nullptr;
static lv_obj_t* _form       = nullptr;
static lv_obj_t* _mic_btn    = nullptr;
static lv_obj_t* _status_lbl = nullptr;
static lv_obj_t* _ta_title   = nullptr;
static lv_obj_t* _ta_dt      = nullptr;
static lv_obj_t* _ta_advance = nullptr;

// ─ Storage ────────────────────────────────────────────────────
static void _load() {
    _reminders.clear();
    if (!SPIFFS.exists(STORAGE_PATH)) return;
    File f = SPIFFS.open(STORAGE_PATH, "r"); if (!f) return;
    DynamicJsonDocument doc(8192);
    if (deserializeJson(doc, f) != DeserializationError::Ok) { f.close(); return; }
    f.close();
    for (JsonObject o : doc.as<JsonArray>()) {
        Reminder r{};
        strlcpy(r.id,          o["id"]         | "", sizeof(r.id));
        strlcpy(r.title,       o["title"]       | "", sizeof(r.title));
        strlcpy(r.description, o["description"] | "", sizeof(r.description));
        strlcpy(r.datetime,    o["datetime"]    | "", sizeof(r.datetime));
        r.advance_min = o["advance_min"] | 15;
        r.done        = o["done"]        | false;
        _reminders.push_back(r);
    }
}

static void _save() {
    File f = SPIFFS.open(STORAGE_PATH, "w"); if (!f) return;
    DynamicJsonDocument doc(8192);
    JsonArray arr = doc.to<JsonArray>();
    for (auto& r : _reminders) {
        JsonObject o = arr.createNestedObject();
        o["id"] = r.id; o["title"] = r.title; o["description"] = r.description;
        o["datetime"] = r.datetime; o["advance_min"] = r.advance_min; o["done"] = r.done;
    }
    serializeJson(doc, f); f.close();
}

// ─ List ──────────────────────────────────────────────────────────
static void _refresh_list() {
    if (!_list) return;
    lv_obj_clean(_list);
    bool any = false;
    for (size_t i = 0; i < _reminders.size(); i++) {
        auto& r = _reminders[i];
        if (r.done) continue;
        any = true;
        lv_obj_t* row = lv_list_add_btn(_list, LV_SYMBOL_BELL, r.title);
        lv_obj_set_user_data(row, (void*)(uintptr_t)i);
        lv_obj_t* sub = lv_label_create(row);
        char sub_txt[72];
        snprintf(sub_txt, sizeof(sub_txt), "%s  \xe2\x88\x92%d min", r.datetime, r.advance_min);
        lv_label_set_text(sub, sub_txt);
        lv_obj_set_style_text_font(sub, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(sub, lv_color_hex(0x888888), 0);
        lv_obj_add_event_cb(row, [](lv_event_t* e) {
            size_t idx = (size_t)(uintptr_t)lv_obj_get_user_data(lv_event_get_target(e));
            if (idx < _reminders.size()) {
                _reminders[idx].done = true; _save(); _refresh_list();
            }
        }, LV_EVENT_CLICKED, nullptr);
    }
    if (!any) {
        lv_obj_t* lbl = lv_label_create(_list);
        lv_label_set_text(lbl, "Aucun rappel.\nAjoute-en un !");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x888888), 0);
    }
}

// ─ Form ──────────────────────────────────────────────────────────
static void _show_form() {
    if (_form) { lv_obj_del(_form); _form = nullptr; }
    _form = lv_obj_create(_screen);
    lv_obj_set_size(_form, 440, 290);
    lv_obj_align(_form, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(_form, lv_color_hex(0x1e1e2e), 0);
    lv_obj_set_style_radius(_form, 12, 0);
    lv_obj_set_style_pad_all(_form, 12, 0);

    auto make_label = [](lv_obj_t* parent, const char* txt, lv_coord_t y) {
        lv_obj_t* l = lv_label_create(parent);
        lv_label_set_text(l, txt);
        lv_obj_align(l, LV_ALIGN_TOP_LEFT, 0, y);
        return l;
    };
    auto make_ta = [](lv_obj_t* parent, lv_coord_t w, lv_coord_t y, const char* ph, bool one_line) {
        lv_obj_t* ta = lv_textarea_create(parent);
        lv_obj_set_size(ta, w, 36); lv_obj_align(ta, LV_ALIGN_TOP_LEFT, 0, y);
        lv_textarea_set_placeholder_text(ta, ph);
        lv_textarea_set_one_line(ta, one_line);
        return ta;
    };
    make_label(_form, "Nouveau rappel", 0);
    make_label(_form, "Titre", 26);
    _ta_title   = make_ta(_form, 300, 42,  "Ex: Rdv m\xc3\xa9decin", true);
    make_label(_form, "Date/heure (YYYY-MM-DDTHH:MM)", 88);
    _ta_dt      = make_ta(_form, 300, 104, "2026-05-20T09:00", true);
    make_label(_form, "Rappel avant (min)", 150);
    _ta_advance = make_ta(_form, 80,  166, "15", true);
    lv_textarea_set_text(_ta_advance, "15");

    lv_obj_t* save_btn = lv_btn_create(_form);
    lv_obj_set_size(save_btn, 84, 36);
    lv_obj_align(save_btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_t* sl = lv_label_create(save_btn);
    lv_label_set_text(sl, LV_SYMBOL_OK " OK"); lv_obj_center(sl);
    lv_obj_add_event_cb(save_btn, [](lv_event_t*) {
        Reminder r{};
        snprintf(r.id, sizeof(r.id), "%lu", (unsigned long)millis());
        strlcpy(r.title,    lv_textarea_get_text(_ta_title),   sizeof(r.title));
        strlcpy(r.datetime, lv_textarea_get_text(_ta_dt),      sizeof(r.datetime));
        r.advance_min = atoi(lv_textarea_get_text(_ta_advance));
        r.done = false;
        _reminders.push_back(r); _save(); _refresh_list();
        if (_form) { lv_obj_del(_form); _form = nullptr; }
    }, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* cancel_btn = lv_btn_create(_form);
    lv_obj_set_size(cancel_btn, 84, 36);
    lv_obj_align(cancel_btn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0x444455), 0);
    lv_obj_t* cl = lv_label_create(cancel_btn);
    lv_label_set_text(cl, LV_SYMBOL_CLOSE); lv_obj_center(cl);
    lv_obj_add_event_cb(cancel_btn, [](lv_event_t*) {
        if (_form) { lv_obj_del(_form); _form = nullptr; }
    }, LV_EVENT_CLICKED, nullptr);
}

// ─ Lifecycle ──────────────────────────────────────────────────
void init() { _load(); }

void start() {
    _screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x0f0f1a), 0);
    lv_scr_load(_screen);

    // Header
    lv_obj_t* hdr = lv_obj_create(_screen);
    lv_obj_set_size(hdr, 480, 44); lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 36);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_t* hdr_lbl = lv_label_create(hdr);
    lv_label_set_text(hdr_lbl, LV_SYMBOL_BELL "  Rappels");
    lv_obj_set_style_text_font(hdr_lbl, &lv_font_montserrat_16, 0);
    lv_obj_center(hdr_lbl);

    // List
    _list = lv_list_create(_screen);
    lv_obj_set_size(_list, 480, 270); lv_obj_align(_list, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_style_bg_color(_list, lv_color_hex(0x12121f), 0);
    lv_obj_set_style_border_width(_list, 0, 0);
    _refresh_list();

    // Toolbar
    lv_obj_t* tb = lv_obj_create(_screen);
    lv_obj_set_size(tb, 480, 52); lv_obj_align(tb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(tb, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_border_width(tb, 0, 0); lv_obj_set_style_pad_hor(tb, 16, 0);
    lv_obj_set_flex_flow(tb, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tb, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* add_btn = lv_btn_create(tb);
    lv_obj_set_size(add_btn, 44, 40);
    lv_obj_set_style_bg_color(add_btn, lv_color_hex(0x4caf50), 0);
    lv_obj_set_style_radius(add_btn, 8, 0);
    lv_obj_t* al = lv_label_create(add_btn); lv_label_set_text(al, LV_SYMBOL_PLUS); lv_obj_center(al);
    lv_obj_add_event_cb(add_btn, [](lv_event_t*){ _show_form(); }, LV_EVENT_CLICKED, nullptr);

    _mic_btn = lv_btn_create(tb);
    lv_obj_set_size(_mic_btn, 44, 40);
    lv_obj_set_style_bg_color(_mic_btn, lv_color_hex(0x2196f3), 0);
    lv_obj_set_style_radius(_mic_btn, 8, 0);
    lv_obj_t* ml = lv_label_create(_mic_btn); lv_label_set_text(ml, LV_SYMBOL_AUDIO); lv_obj_center(ml);
    lv_obj_add_event_cb(_mic_btn, [](lv_event_t*){
        voice::start_recording();
        lv_obj_set_style_bg_color(_mic_btn, lv_color_hex(0xff9800), 0);
    }, LV_EVENT_CLICKED, nullptr);

    _status_lbl = lv_label_create(_screen);
    lv_label_set_text(_status_lbl, "");
    lv_obj_align(_status_lbl, LV_ALIGN_BOTTOM_MID, 0, -56);
    lv_obj_set_style_text_font(_status_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(_status_lbl, lv_color_hex(0x888888), 0);
}

void stop() {
    if (!_screen) return;
    lv_obj_del(_screen);
    _screen = _list = _form = _mic_btn = _status_lbl = nullptr;
    _ta_title = _ta_dt = _ta_advance = nullptr;
}

void tick() {
    time_t now = time(nullptr);
    for (auto& r : _reminders) {
        if (r.done) continue;
        struct tm tm_r{};
        if (sscanf(r.datetime, "%d-%d-%dT%d:%d",
            &tm_r.tm_year, &tm_r.tm_mon, &tm_r.tm_mday,
            &tm_r.tm_hour, &tm_r.tm_min) != 5) continue;
        tm_r.tm_year -= 1900; tm_r.tm_mon -= 1; tm_r.tm_sec = 0;
        time_t t_remind = mktime(&tm_r) - r.advance_min * 60;
        if (now >= t_remind && now < t_remind + 60) {
            char msg[160];
            snprintf(msg, sizeof(msg), "Rappel : %s", r.title);
            voice::speak(msg);
            if (_status_lbl && _screen && lv_scr_act() == _screen) {
                lv_label_set_text(_status_lbl, msg);
                lv_obj_set_style_text_color(_status_lbl, lv_color_hex(0xff9800), 0);
            }
            r.done = true; _save();
        }
    }
    if (_mic_btn && !voice::wake_word_detected())
        lv_obj_set_style_bg_color(_mic_btn, lv_color_hex(0x2196f3), 0);
}

void handle_voice_intent(const char* text) {
    Reminder r{};
    snprintf(r.id, sizeof(r.id), "%lu", (unsigned long)millis());
    strlcpy(r.title, text, sizeof(r.title));
    time_t now = time(nullptr); struct tm* t = localtime(&now);
    t->tm_mday += 1; t->tm_hour = 9; t->tm_min = 0; mktime(t);
    strftime(r.datetime, sizeof(r.datetime), "%Y-%m-%dT%H:%M:00", t);
    r.advance_min = 15; r.done = false;
    _reminders.push_back(r); _save();
    if (_list) _refresh_list();
    char confirm[160];
    snprintf(confirm, sizeof(confirm), "Rappel cr\xc3\xa9\xc3\xa9 : %s", r.title);
    voice::speak(confirm);
}

void sync_from_pwa(const char* json_array) {
    _reminders.clear();
    DynamicJsonDocument doc(8192);
    if (deserializeJson(doc, json_array) != DeserializationError::Ok) return;
    for (JsonObject o : doc.as<JsonArray>()) {
        Reminder r{};
        strlcpy(r.id, o["id"]|"", sizeof(r.id));
        strlcpy(r.title, o["title"]|"", sizeof(r.title));
        strlcpy(r.description, o["description"]|"", sizeof(r.description));
        strlcpy(r.datetime, o["datetime"]|"", sizeof(r.datetime));
        r.advance_min = o["advance_min"]|15; r.done = o["done"]|false;
        _reminders.push_back(r);
    }
    _save(); if (_list) _refresh_list();
}

const char* get_json_list() {
    static char buf[8192];
    DynamicJsonDocument doc(8192);
    JsonArray arr = doc.to<JsonArray>();
    for (auto& r : _reminders) {
        JsonObject o = arr.createNestedObject();
        o["id"]=r.id; o["title"]=r.title; o["description"]=r.description;
        o["datetime"]=r.datetime; o["advance_min"]=r.advance_min; o["done"]=r.done;
    }
    serializeJson(doc, buf, sizeof(buf));
    return buf;
}

} // reminders
} // apps
