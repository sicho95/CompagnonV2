// ============================================================
// CompagnonV2 — apps/app_rappels.cpp
// Fix 4 : #include <lvgl.h> présent en tête
//         cast (void*)(intptr_t) — correct LVGL9 / 64-bit
//         hal::audio::playTone supprimé → voice::speak
// Fix 5 : #include "../voice/voice_engine.h" unifié
// ============================================================
#include "app_rappels.h"
#include "../system/scheduler.h"
#include "../storage/nvs_store.h"
#include "../voice/voice_engine.h"  // Fix 5 — header unifié voice::
#include <lvgl.h>                   // Fix 4 — lv_obj_t, lv_event_t, etc.
#include <Arduino.h>
#include <time.h>

static lv_obj_t* _screen    = nullptr;
static lv_obj_t* _list      = nullptr;
static lv_obj_t* _lbl_empty = nullptr;

static void _rebuild_list();

static void _on_delete_btn(lv_event_t* e) {
    // Fix 4 : cast (void*)(intptr_t) → int, correct sous LVGL9
    int id = (int)(intptr_t)lv_event_get_user_data(e);
    Scheduler::cancelAlarm(id);
    ReminderStore::remove(id);
    _rebuild_list();
    voice::speak("Rappel supprimé.");  // Fix 4 : plus de hal::audio::playTone
}

static void _rebuild_list() {
    if (!_list) return;
    lv_obj_clean(_list);
    const auto& all = ReminderStore::getAll();
    if (all.empty()) {
        lv_obj_clear_flag(_lbl_empty, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_add_flag(_lbl_empty, LV_OBJ_FLAG_HIDDEN);
    for (const auto& r : all) {
        lv_obj_t* row = lv_obj_create(_list);
        lv_obj_set_size(row, LV_PCT(100), 52);
        lv_obj_set_style_bg_color(row, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(0x1A1A1A), 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_radius(row, 0, 0);
        lv_obj_set_style_pad_all(row, 8, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* ico = lv_label_create(row);
        lv_label_set_text(ico, r.enabled ? LV_SYMBOL_BELL : LV_SYMBOL_MUTE);
        lv_obj_set_style_text_color(ico,
            r.enabled ? lv_color_hex(0xFFD700) : lv_color_hex(0x222222), 0);
        lv_obj_set_style_text_font(ico, &lv_font_montserrat_20, 0);
        lv_obj_set_style_pad_right(ico, 10, 0);

        lv_obj_t* col = lv_obj_create(row);
        lv_obj_set_flex_grow(col, 1);
        lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(col, 0, 0);
        lv_obj_set_style_pad_all(col, 0, 0);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);

        lv_obj_t* lbl_text = lv_label_create(col);
        lv_label_set_long_mode(lbl_text, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(lbl_text, 290);
        lv_label_set_text(lbl_text, r.label.c_str());
        lv_obj_set_style_text_color(lbl_text, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl_text, &lv_font_montserrat_14, 0);

        char timebuf[24];
        struct tm lt;
        localtime_r(&r.datetime, &lt);
        snprintf(timebuf, sizeof(timebuf), "%02d/%02d  %02dh%02d",
                 lt.tm_mday, lt.tm_mon + 1, lt.tm_hour, lt.tm_min);
        lv_obj_t* lbl_time = lv_label_create(col);
        lv_label_set_text(lbl_time, timebuf);
        lv_obj_set_style_text_color(lbl_time, lv_color_hex(0x444444), 0);
        lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_12, 0);

        lv_obj_t* del_btn = lv_btn_create(row);
        lv_obj_set_size(del_btn, 36, 36);
        lv_obj_set_style_bg_color(del_btn, lv_color_hex(0x6B0000), 0);
        lv_obj_set_style_radius(del_btn, 8, 0);
        lv_obj_set_style_pad_all(del_btn, 4, 0);
        lv_obj_t* del_ico = lv_label_create(del_btn);
        lv_label_set_text(del_ico, LV_SYMBOL_TRASH);
        lv_obj_set_style_text_color(del_ico, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(del_ico);
        lv_obj_add_event_cb(del_btn, _on_delete_btn, LV_EVENT_CLICKED,
                            (void*)(intptr_t)r.id);
    }
}

static time_t _parse_reminder_time(const String& text) {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    int idx = text.indexOf("dans ");
    if (idx >= 0) {
        int num = text.substring(idx + 5).toInt();
        if (num > 0) {
            if (text.indexOf("heure") >= 0) return now + num * 3600;
            return now + num * 60;
        }
    }
    bool demain = (text.indexOf("demain") >= 0);
    if (demain || text.indexOf("aujourd") >= 0) {
        for (int i = 0; i < (int)text.length(); i++) {
            if (isDigit(text[i])) {
                int num  = text.substring(i).toInt();
                int nlen = String(num).length();
                char after = (i + nlen < (int)text.length()) ? text[i + nlen] : ' ';
                if (after == 'h' || after == 'H' || after == ':') {
                    t.tm_hour = num; t.tm_min = 0; t.tm_sec = 0;
                    int j = i + nlen + 1;
                    if (j < (int)text.length() && isDigit(text[j]))
                        t.tm_min = text.substring(j).toInt();
                    if (demain) t.tm_mday += 1;
                    return mktime(&t);
                }
            }
        }
    }
    return now + 3600;
}

void AppRappels::init() {
    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, 0);

    lv_obj_t* header = lv_obj_create(_screen);
    lv_obj_set_size(header, 480, 44);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 36);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(header, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_pad_all(header, 8, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, LV_SYMBOL_BELL "  Rappels");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFD700), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 8, 0);

    lv_obj_t* hint = lv_label_create(header);
    lv_label_set_text(hint, LV_SYMBOL_AUDIO " \"Rappelle-moi...\"");
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x333333), 0);
    lv_obj_align(hint, LV_ALIGN_RIGHT_MID, -8, 0);

    _list = lv_obj_create(_screen);
    lv_obj_set_size(_list, 480, LV_VER_RES - 36 - 44 - 8);
    lv_obj_align(_list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(_list, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(_list, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_list, 0, 0);
    lv_obj_set_style_pad_row(_list, 0, 0);
    lv_obj_set_style_pad_all(_list, 0, 0);
    lv_obj_set_flex_flow(_list, LV_FLEX_FLOW_COLUMN);

    _lbl_empty = lv_label_create(_screen);
    lv_label_set_text(_lbl_empty,
        LV_SYMBOL_BELL "\nAucun rappel\n\nDites : \"Rappelle-moi...\"");
    lv_obj_set_style_text_align(_lbl_empty, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(_lbl_empty, lv_color_hex(0x333333), 0);
    lv_obj_set_style_text_font(_lbl_empty, &lv_font_montserrat_16, 0);
    lv_obj_align(_lbl_empty, LV_ALIGN_CENTER, 0, 20);

    Serial.println("[Rappels] init");
}

void AppRappels::onResume() {
    _rebuild_list();
    lv_scr_load_anim(_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}
void AppRappels::update() {}
void AppRappels::onPause() {}

void AppRappels::handleIntent(const char* intent, const char* param) {
    String text(param);
    if (strcmp(intent, "create_reminder") == 0) {
        Reminder r;
        r.label           = text;
        r.datetime        = _parse_reminder_time(text);
        r.advance_minutes = 0;
        r.enabled         = true;
        if (ReminderStore::add(r)) {
            Scheduler::scheduleReminder(r);
            _rebuild_list();
            struct tm lt; localtime_r(&r.datetime, &lt);
            char confirm[128];
            snprintf(confirm, sizeof(confirm),
                     "Rappel créé pour le %d à %02dh%02d.",
                     lt.tm_mday, lt.tm_hour, lt.tm_min);
            voice::speak(confirm);
        }
    } else if (strcmp(intent, "delete_reminder") == 0) {
        int id = atoi(param);
        Scheduler::cancelAlarm(id);
        ReminderStore::remove(id);
        _rebuild_list();
        voice::speak("Rappel supprimé.");
    } else if (strcmp(intent, "list_reminders") == 0) {
        auto all = ReminderStore::getAll();
        if (all.empty()) {
            voice::speak("Vous n'avez aucun rappel programmé.");
        } else {
            String msg = String((int)all.size()) + " rappel";
            if (all.size() > 1) msg += "s";
            msg += " en attente.";
            voice::speak(msg.c_str());
        }
    }
}

time_t AppRappels::nextEpoch() const {
    time_t earliest = 0;
    for (const auto& r : ReminderStore::getAll()) {
        if (!r.enabled) continue;
        time_t trigger = r.datetime - (time_t)(r.advance_minutes * 60);
        if (trigger > time(nullptr)) {
            if (earliest == 0 || trigger < earliest) earliest = trigger;
        }
    }
    return earliest;
}
