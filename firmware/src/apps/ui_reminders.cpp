// ============================================================
// CompagnonV2 — App Rappels (LVGL 9.x)
// ============================================================
#include "ui_reminders.h"
#include "../voice/voice_engine.h"
#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>   // fix: JsonDocument (ArduinoJson 7, DynamicJsonDocument déprecié)
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
    // fix: JsonDocument remplace DynamicJsonDocument (ArduinoJson 7)
    JsonDocument doc;
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
    // fix: JsonDocument remplace DynamicJsonDocument (ArduinoJson 7)
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (auto& r : _reminders) {
        // fix: arr.add<JsonObject>() remplace arr.createNestedObject() (ArduinoJson 7)
        JsonObject o = arr.add<JsonObject>();
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
            lv_obj_t* target = (lv_obj_t*)lv_event_get_target(e);
            size_t idx = (size_t)(uintptr_t)lv_obj_get_user_data(target);
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
    if (_form) { lv_obj_delete(_form); _form = nullptr; }
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
        if (_form) { lv_obj_delete(_form); _form = nullptr; }
    }, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* cancel_btn = lv_btn_create(_form);
    lv_obj_set_size(cancel_btn, 84, 36);
    lv_obj_align(cancel_btn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0x444455), 0);
    lv_obj_t* cl = lv_label_create(cancel_btn);
    lv_label_set_text(cl, LV_SYMBOL_CLOSE); lv_obj_center(cl);
    lv_obj_add_event_cb(cancel_btn, [](lv_event_t*) {
        if (_form) { lv_obj_delete(_form); _form = nullptr; }
    }, LV_EVENT_CLICKED, nullptr);
}

// ─ Lifecycle ──────────────────────────────────────────────────
void init() { _load(); }

void start() {
    if (_screen) return;
    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x12121e), 0);
    lv_scr_load(_screen);

    lv_obj_t* title = lv_label_create(_screen);
    lv_label_set_text(title, LV_SYMBOL_BELL "  Rappels");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 12, 10);

    lv_obj_t* add_btn = lv_btn_create(_screen);
    lv_obj_set_size(add_btn, 36, 36);
    lv_obj_align(add_btn, LV_ALIGN_TOP_RIGHT, -12, 6);
    lv_obj_t* add_lbl = lv_label_create(add_btn);
    lv_label_set_text(add_lbl, LV_SYMBOL_PLUS); lv_obj_center(add_lbl);
    lv_obj_add_event_cb(add_btn, [](lv_event_t*) { _show_form(); }, LV_EVENT_CLICKED, nullptr);

    _mic_btn = lv_btn_create(_screen);
    lv_obj_set_size(_mic_btn, 36, 36);
    lv_obj_align(_mic_btn, LV_ALIGN_TOP_RIGHT, -54, 6);
    lv_obj_set_style_bg_color(_mic_btn, lv_color_hex(0x333355), 0);
    lv_obj_t* mic_lbl = lv_label_create(_mic_btn);
    lv_label_set_text(mic_lbl, LV_SYMBOL_AUDIO); lv_obj_center(mic_lbl);
    lv_obj_add_event_cb(_mic_btn, [](lv_event_t*) {
        voice_engine_start_recording();  // fix: voice::start_recording() → voice_engine_start_recording()
    }, LV_EVENT_CLICKED, nullptr);

    _status_lbl = lv_label_create(_screen);
    lv_label_set_text(_status_lbl, "");
    lv_obj_set_style_text_color(_status_lbl, lv_color_hex(0x88aaff), 0);
    lv_obj_align(_status_lbl, LV_ALIGN_TOP_MID, 0, 48);

    _list = lv_list_create(_screen);
    lv_obj_set_size(_list, 460, 230);
    lv_obj_align(_list, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_set_style_bg_color(_list, lv_color_hex(0x1a1a2e), 0);
    _refresh_list();
}

void stop() {
    if (_screen) { lv_obj_delete(_screen); _screen = nullptr; _list = nullptr;
                   _form = nullptr; _mic_btn = nullptr; _status_lbl = nullptr; }
}

void tick() {
    // Vérification déclenchement rappels
    time_t now = time(nullptr);
    if (now < 1000000) return;
    for (auto& r : _reminders) {
        if (r.done) continue;
        struct tm tm_ev = {};
        strptime(r.datetime, "%Y-%m-%dT%H:%M", &tm_ev);
        time_t t_ev  = mktime(&tm_ev);
        time_t t_wup = t_ev - (time_t)(r.advance_min * 60);
        if (now >= t_wup && now < t_wup + 60) {
            char msg[160];
            snprintf(msg, sizeof(msg), "Rappel : %s.", r.title);
            voice_engine_speak(msg);
            r.done = true; _save(); _refresh_list();
        }
    }
    // Indicateur micro actif (wake word)
    if (_mic_btn && !voice_engine_wake_word_detected()) {
        lv_obj_set_style_bg_color(_mic_btn, lv_color_hex(0x333355), 0);
    } else if (_mic_btn) {
        lv_obj_set_style_bg_color(_mic_btn, lv_color_hex(0x883333), 0);
    }
}

void handle_voice_intent(const char* intent_json) {
    char confirm[128];
    snprintf(confirm, sizeof(confirm), "Compris : %s", intent_json);
    voice_engine_speak(confirm);
}

} // namespace reminders
} // namespace apps
