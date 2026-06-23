// ============================================================
// reminder_app.cpp — App Rappels CompagnonV2
//
// CRUD persisté dans /reminders.json (SPIFFS / FATFS)
// Scheduler : app_tick() appelé chaque 1 s par OS main
// Déclenchement : son + TTS + UI modale LVGL (TODO)
// Sync BLE : sérialisation/désérialisation JSON
// ============================================================
#include "reminder_app.h"
#include "../../voice/voice_engine.h"
#include "../../hal/hal_audio.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <Arduino.h>
#include <time.h>

namespace apps::reminders {

static const char* STORAGE_PATH = "/reminders.json";
static JsonDocument _doc;
static bool _loaded = false;

static void _load() {
    if (_loaded) return;
    _doc.clear();
    if (SPIFFS.exists(STORAGE_PATH)) {
        File f = SPIFFS.open(STORAGE_PATH, "r");
        if (f) { deserializeJson(_doc, f); f.close(); }
    }
    if (!_doc["reminders"].is<JsonArray>()) _doc["reminders"].to<JsonArray>();
    _loaded = true;
}

static void _save() {
    File f = SPIFFS.open(STORAGE_PATH, "w");
    if (f) { serializeJson(_doc, f); f.close(); }
}

static void _gen_uuid(char* out) {
    uint32_t r[4] = { esp_random(), esp_random(), esp_random(), esp_random() };
    r[1] = (r[1] & 0xffff0fff) | 0x00004000;
    r[2] = (r[2] & 0x3fffffff) | 0x80000000;
    snprintf(out, 37, "%08x-%04x-%04x-%04x-%04x%08x",
        r[0], (r[1]>>16)&0xffff, r[1]&0xffff,
        (r[2]>>16)&0xffff, r[2]&0xffff, r[3]);
}

void app_init()  { _load(); }
void app_start() { Serial.println("[Rappels] app_start — UI TODO"); }
void app_stop()  { Serial.println("[Rappels] app_stop"); }

void app_tick() {
    time_t now = time(nullptr);
    if (now < 1000000) return;
    Reminder next = reminder_next();
    if (next.active && !next.done && now >= (time_t)next.epoch_trigger)
        reminder_trigger(next.id);
}

bool reminder_add(const Reminder& r) {
    _load();
    JsonArray arr = _doc["reminders"].as<JsonArray>();
    JsonObject obj = arr.add<JsonObject>();
    obj["id"]            = r.id;
    obj["title"]         = r.title;
    obj["description"]   = r.description;
    obj["epoch_trigger"] = r.epoch_trigger;
    obj["advance_s"]     = r.advance_s;
    obj["done"]          = false;
    obj["active"]        = true;
    _save(); return true;
}

bool reminder_delete(const char* id) {
    _load();
    JsonArray arr = _doc["reminders"].as<JsonArray>();
    for (size_t i = 0; i < arr.size(); i++) {
        if (strcmp(arr[i]["id"].as<const char*>(), id) == 0) {
            arr.remove(i); _save(); return true;
        }
    }
    return false;
}

bool reminder_done(const char* id) {
    _load();
    for (JsonObject obj : _doc["reminders"].as<JsonArray>()) {
        if (strcmp(obj["id"].as<const char*>(), id) == 0) {
            obj["done"] = true; _save(); return true;
        }
    }
    return false;
}

uint8_t reminder_list(Reminder* out, uint8_t max_count) {
    _load(); uint8_t n = 0;
    for (JsonObject obj : _doc["reminders"].as<JsonArray>()) {
        if (n >= max_count) break;
        strlcpy(out[n].id,          obj["id"]          | "", 37);
        strlcpy(out[n].title,       obj["title"]       | "", 64);
        strlcpy(out[n].description, obj["description"] | "", 128);
        out[n].epoch_trigger = obj["epoch_trigger"] | 0U;
        out[n].advance_s     = obj["advance_s"]     | 0U;
        out[n].done          = obj["done"]          | false;
        out[n].active        = obj["active"]        | true;
        n++;
    }
    return n;
}

Reminder reminder_next() {
    Reminder res = {};
    _load();
    time_t now = time(nullptr);
    time_t soonest = LONG_MAX;
    for (JsonObject obj : _doc["reminders"].as<JsonArray>()) {
        if (obj["done"] | false) continue;
        if (!(obj["active"] | true)) continue;
        time_t t = obj["epoch_trigger"] | 0;
        if (t > 0 && t < soonest && t >= now) {
            soonest = t;
            strlcpy(res.id,          obj["id"]          | "", 37);
            strlcpy(res.title,       obj["title"]       | "", 64);
            strlcpy(res.description, obj["description"] | "", 128);
            res.epoch_trigger = (uint32_t)t;
            res.advance_s     = obj["advance_s"] | 0U;
            res.done   = false; res.active = true;
        }
    }
    return res;
}

void reminder_trigger(const char* id) {
    _load();
    String title, desc;
    for (JsonObject obj : _doc["reminders"].as<JsonArray>()) {
        if (strcmp(obj["id"].as<const char*>(), id) == 0) {
            title = obj["title"].as<String>();
            desc  = obj["description"].as<String>();
            obj["done"] = true; _save(); break;
        }
    }
    if (title.isEmpty()) return;

    // fix: voice:: inexistant — utiliser voice_engine_is_silent() + voice_engine_speak()
    // fix: hal::audio_play_tone inexistant — hal::audio_play_pcm (bip synthétique via PCM)
    if (!voice_engine_is_silent()) {
        // Bip d'alerte 880 Hz / 500 ms via PCM (voir hal_audio pour la génération)
        // hal::audio_play_pcm(nullptr, 0);  // TODO: passer un buffer PCM bip 880Hz
        Serial.println("[Rappels] bip alerte");
    }
    String tts_msg = "Rappel : " + title + (desc.length() ? ". " + desc : "");
    voice_engine_speak(tts_msg.c_str());

    // TODO: afficher modale LVGL
    Serial.printf("[Rappels] TRIGGER '%s'\n", title.c_str());
}

String reminder_to_json_all() {
    _load(); String out; serializeJson(_doc, out); return out;
}

bool reminder_from_json(const String& json) {
    JsonDocument incoming;
    if (deserializeJson(incoming, json)) return false;
    JsonArray in_arr = incoming["reminders"].as<JsonArray>();
    JsonArray my_arr = _doc["reminders"].as<JsonArray>();
    for (JsonObject in_obj : in_arr) {
        const char* in_id = in_obj["id"] | "";
        bool found = false;
        for (JsonObject my_obj : my_arr) {
            if (strcmp(my_obj["id"].as<const char*>(), in_id) == 0) {
                my_obj.set(in_obj); found = true; break;
            }
        }
        if (!found) my_arr.add<JsonObject>().set(in_obj);
    }
    _save(); return true;
}

} // namespace apps::reminders
