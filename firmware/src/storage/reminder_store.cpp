// ============================================================
// CompagnonV2 — storage/reminder_store.cpp
// ArduinoJson v7 + FATFS (FFat)
// ============================================================
#include "reminder_store.h"
#include <ArduinoJson.h>
#include <FFat.h>

#define REMINDERS_FILE "/reminders.json"

namespace ReminderStore {

static std::vector<Reminder> _reminders;
static int _nextId = 1;

bool init() {
    if (!FFat.exists("/")) {
        Serial.println("[REMINDER_STORE] FFat not mounted");
        return false;
    }
    return load();
}

bool load() {
    _reminders.clear();
    _nextId = 1;

    if (!FFat.exists(REMINDERS_FILE)) {
        Serial.println("[REMINDER_STORE] No file yet, starting fresh");
        return true;
    }

    File f = FFat.open(REMINDERS_FILE, FILE_READ);
    if (!f) {
        Serial.println("[REMINDER_STORE] Cannot open file");
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err) {
        Serial.printf("[REMINDER_STORE] JSON parse error: %s\n", err.c_str());
        return false;
    }

    JsonArray arr = doc["reminders"].as<JsonArray>();
    for (JsonObject obj : arr) {
        Reminder r;
        r.id               = obj["id"]             | 0;
        r.label            = obj["label"]           | String("");
        r.datetime         = obj["datetime"]        | (time_t)0;
        r.advance_minutes  = obj["advance_minutes"] | 0;
        r.enabled          = obj["enabled"]         | true;
        _reminders.push_back(r);
        if (r.id >= _nextId) _nextId = r.id + 1;
    }
    Serial.printf("[REMINDER_STORE] Loaded %d reminders\n", (int)_reminders.size());
    return true;
}

bool save() {
    JsonDocument doc;
    JsonArray arr = doc["reminders"].to<JsonArray>();
    for (const auto& r : _reminders) {
        JsonObject obj = arr.add<JsonObject>();
        obj["id"]              = r.id;
        obj["label"]           = r.label;
        obj["datetime"]        = (long long)r.datetime;
        obj["advance_minutes"] = r.advance_minutes;
        obj["enabled"]         = r.enabled;
    }

    File f = FFat.open(REMINDERS_FILE, FILE_WRITE);
    if (!f) {
        Serial.println("[REMINDER_STORE] Cannot write file");
        return false;
    }
    serializeJson(doc, f);
    f.close();
    Serial.printf("[REMINDER_STORE] Saved %d reminders\n", (int)_reminders.size());
    return true;
}

int add(const Reminder& r) {
    Reminder nr = r;
    nr.id = _nextId++;
    _reminders.push_back(nr);
    save();
    return nr.id;
}

bool remove(int id) {
    for (auto it = _reminders.begin(); it != _reminders.end(); ++it) {
        if (it->id == id) {
            _reminders.erase(it);
            save();
            return true;
        }
    }
    return false;
}

bool update(const Reminder& r) {
    for (auto& existing : _reminders) {
        if (existing.id == r.id) {
            existing = r;
            save();
            return true;
        }
    }
    return false;
}

const std::vector<Reminder>& getAll() {
    return _reminders;
}

const Reminder* getById(int id) {
    for (const auto& r : _reminders) {
        if (r.id == id) return &r;
    }
    return nullptr;
}

// fix: implémentation manquante de findById (déclarée dans .h mais absente)
std::optional<Reminder> findById(int id) {
    for (const auto& r : _reminders) {
        if (r.id == id) return r;
    }
    return std::nullopt;
}

time_t nextTriggerEpoch() {
    time_t now = time(nullptr);
    time_t best = 0;
    for (const auto& r : _reminders) {
        if (!r.enabled) continue;
        time_t trigger = r.datetime - (time_t)(r.advance_minutes * 60);
        if (trigger <= now) continue;
        if (best == 0 || trigger < best) best = trigger;
    }
    return best;
}

} // namespace ReminderStore
