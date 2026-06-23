#include "reminders_app.h"
#include "../../voice/voice_engine.h"
#include "../../system/sd_mgr.h"
#include "../../config/nvs_config.h"
#include <lvgl.h>          // fix: manquant — requis pour lv_obj_t, lv_obj_delete
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <time.h>
#include <string.h>

static const char *TAG = "REMINDERS";
static const char *REMINDERS_PATH = "/reminders/reminders.json";

#define MAX_REMINDERS 64

typedef struct {
    char  id[37];           // UUID v4
    char  title[64];
    char  description[128];
    char  datetime[20];     // ISO 8601 "2026-05-20T14:00:00"
    int   advance_minutes;
    char  status[12];       // scheduled|done|cancelled|snoozed
    int   snooze_minutes;
    char  created_at[20];
    char  updated_at[20];
} Reminder;

static Reminder  s_reminders[MAX_REMINDERS];
static int       s_count = 0;
static lv_obj_t *s_screen = nullptr;

// ─── Persistance ─────────────────────────────────────────────────────────────
static void load_from_storage(void) {
    char *buf = sd_mgr_read_file(REMINDERS_PATH);
    if (!buf) { ESP_LOGW(TAG, "Pas de fichier rappels"); return; }

    JsonDocument doc;
    if (deserializeJson(doc, buf)) { free(buf); return; }
    free(buf);

    JsonArray arr = doc.as<JsonArray>();
    s_count = 0;
    for (JsonObject obj : arr) {
        if (s_count >= MAX_REMINDERS) break;
        Reminder &r = s_reminders[s_count++];
        strlcpy(r.id,              obj["id"]          | "", sizeof(r.id));
        strlcpy(r.title,           obj["title"]        | "", sizeof(r.title));
        strlcpy(r.description,     obj["description"]  | "", sizeof(r.description));
        strlcpy(r.datetime,        obj["datetime"]     | "", sizeof(r.datetime));
        r.advance_minutes = obj["advance_minutes"] | 15;
        strlcpy(r.status,          obj["status"]       | "scheduled", sizeof(r.status));
        r.snooze_minutes  = obj["snooze_minutes"]  | 10;
        strlcpy(r.created_at,      obj["created_at"]   | "", sizeof(r.created_at));
        strlcpy(r.updated_at,      obj["updated_at"]   | "", sizeof(r.updated_at));
    }
    ESP_LOGI(TAG, "%d rappel(s) chargé(s)", s_count);
}

static void save_to_storage(void) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < s_count; i++) {
        JsonObject obj = arr.add<JsonObject>();
        obj["id"]              = s_reminders[i].id;
        obj["title"]           = s_reminders[i].title;
        obj["description"]     = s_reminders[i].description;
        obj["datetime"]        = s_reminders[i].datetime;
        obj["advance_minutes"] = s_reminders[i].advance_minutes;
        obj["status"]          = s_reminders[i].status;
        obj["snooze_minutes"]  = s_reminders[i].snooze_minutes;
        obj["created_at"]      = s_reminders[i].created_at;
        obj["updated_at"]      = s_reminders[i].updated_at;
    }
    char buf[4096];
    serializeJson(doc, buf, sizeof(buf));
    sd_mgr_write_file(REMINDERS_PATH, buf);
}

// ─── Scheduling : prochain wakeup RTC ────────────────────────────────────────
static void schedule_next_wakeup(void) {
    time_t now;
    time(&now);
    time_t next = -1;

    for (int i = 0; i < s_count; i++) {
        if (strcmp(s_reminders[i].status, "scheduled") != 0) continue;
        struct tm tm_ev = {};
        strptime(s_reminders[i].datetime, "%Y-%m-%dT%H:%M:%S", &tm_ev);
        time_t t_ev  = mktime(&tm_ev);
        time_t t_wup = t_ev - (time_t)(s_reminders[i].advance_minutes * 60);
        if (t_wup > now && (next == -1 || t_wup < next))
            next = t_wup;
    }

    if (next > 0) {
        uint64_t sleep_us = (uint64_t)(next - now) * 1000000ULL;
        // esp_sleep_enable_timer_wakeup(sleep_us);
        // NOTE : ne pas appeler esp_light_sleep_start() ici — c'est power_mgr qui décide
        ESP_LOGI(TAG, "Prochain rappel dans %lld s", (long long)(next - now));
    }
}

// ─── Tick : vérification sonnerie ────────────────────────────────────────────
void reminders_app_tick(void) {
    time_t now;
    time(&now);

    for (int i = 0; i < s_count; i++) {
        if (strcmp(s_reminders[i].status, "scheduled") != 0) continue;
        struct tm tm_ev = {};
        strptime(s_reminders[i].datetime, "%Y-%m-%dT%H:%M:%S", &tm_ev);
        time_t t_ev  = mktime(&tm_ev);
        time_t t_wup = t_ev - (time_t)(s_reminders[i].advance_minutes * 60);

        if (now >= t_wup && now < t_wup + 60) {
            ESP_LOGI(TAG, "RAPPEL: %s", s_reminders[i].title);
            char msg[200];
            snprintf(msg, sizeof(msg), "Rappel : %s. %s",
                     s_reminders[i].title, s_reminders[i].description);
            voice_engine_speak(msg);  // TTS (respecte le mode silencieux)
            // TODO : afficher popup LVGL avec options "Fait" / "Plus tard"
            strlcpy(s_reminders[i].status, "done", sizeof(s_reminders[i].status));
            save_to_storage();
            schedule_next_wakeup();
        }
    }
}

// ─── API publique ─────────────────────────────────────────────────────────────
void reminders_app_init(void) {
    load_from_storage();
    schedule_next_wakeup();
    ESP_LOGI(TAG, "Reminders init OK");
}

void reminders_app_start(void) {
    ESP_LOGI(TAG, "reminders_app_start — UI à implémenter");
}

void reminders_app_stop(void) {
    // fix: lv_obj_del supprimé en LVGL 9 → lv_obj_delete
    if (s_screen) { lv_obj_delete(s_screen); s_screen = nullptr; }
}

void reminders_app_ble_set(const char *json_payload) {
    if (!json_payload) return;
    JsonDocument doc;
    if (deserializeJson(doc, json_payload)) return;
    JsonArray arr = doc["reminders"].as<JsonArray>();
    if (!arr) arr = doc.as<JsonArray>();
    s_count = 0;
    for (JsonObject obj : arr) {
        if (s_count >= MAX_REMINDERS) break;
        Reminder &r = s_reminders[s_count++];
        strlcpy(r.id,          obj["id"]          | "", sizeof(r.id));
        strlcpy(r.title,       obj["title"]        | "", sizeof(r.title));
        strlcpy(r.description, obj["description"]  | "", sizeof(r.description));
        strlcpy(r.datetime,    obj["datetime"]     | "", sizeof(r.datetime));
        r.advance_minutes = obj["advance_minutes"] | 15;
        strlcpy(r.status,      obj["status"]       | "scheduled", sizeof(r.status));
        r.snooze_minutes  = obj["snooze_minutes"]  | 10;
        strlcpy(r.created_at,  obj["created_at"]   | "", sizeof(r.created_at));
        strlcpy(r.updated_at,  obj["updated_at"]   | "", sizeof(r.updated_at));
    }
    save_to_storage();
    schedule_next_wakeup();
}

void reminders_app_ble_get(char *out, size_t out_len) {
    JsonDocument doc;
    JsonArray arr = doc["reminders"].to<JsonArray>();
    for (int i = 0; i < s_count; i++) {
        JsonObject obj = arr.add<JsonObject>();
        obj["id"]              = s_reminders[i].id;
        obj["title"]           = s_reminders[i].title;
        obj["description"]     = s_reminders[i].description;
        obj["datetime"]        = s_reminders[i].datetime;
        obj["advance_minutes"] = s_reminders[i].advance_minutes;
        obj["status"]          = s_reminders[i].status;
        obj["snooze_minutes"]  = s_reminders[i].snooze_minutes;
    }
    doc["cmd"] = "get_reminders_ack";
    serializeJson(doc, out, out_len);
}

void reminders_app_create_from_text(const char *natural_text) {
    if (!natural_text || s_count >= MAX_REMINDERS) return;
    Reminder &r = s_reminders[s_count++];
    snprintf(r.id, sizeof(r.id), "voice-%ld", (long)time(nullptr));
    strlcpy(r.title, natural_text, sizeof(r.title));
    strlcpy(r.description, "", sizeof(r.description));
    strlcpy(r.datetime, "2026-01-01T09:00:00", sizeof(r.datetime));
    r.advance_minutes = 15;
    strlcpy(r.status, "scheduled", sizeof(r.status));
    r.snooze_minutes = 10;
    save_to_storage();
    voice_engine_speak("Rappel créé. Pensez à préciser la date depuis l'application.");
    ESP_LOGI(TAG, "Rappel créé depuis la voix: '%s'", natural_text);
}
