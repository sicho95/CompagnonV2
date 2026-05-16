// ============================================================
// CompagnonV2 — system/scheduler.cpp
// Scheduler rappels : RTC alarm + deep sleep
// TTS du label via HttpClient → hal_audio DAC
// ============================================================
#include "scheduler.h"
#include "../storage/reminder_store.h"
#include "../hal/rtc.h"
#include "../hal/hal_audio.h"
#include "../net/http_client.h"
#include <esp_sleep.h>
#include <Arduino.h>
#include <vector>

namespace Scheduler {

struct ScheduledAlarm {
    int    reminder_id;
    time_t trigger_epoch;
};

static std::vector<ScheduledAlarm> _alarms;

void scheduleReminder(const ReminderStore::Reminder& r) {
    if (!r.enabled) return;
    time_t trigger = r.datetime - (time_t)(r.advance_minutes * 60);
    time_t now = time(nullptr);
    if (trigger <= now) {
        Serial.printf("[SCHEDULER] Reminder %d already past, skip\n", r.id);
        return;
    }
    for (auto& a : _alarms) {
        if (a.reminder_id == r.id) { a.trigger_epoch = trigger; return; }
    }
    _alarms.push_back({r.id, trigger});
    Serial.printf("[SCHEDULER] Scheduled reminder %d at epoch %lu\n",
        r.id, (unsigned long)trigger);
}

void rescheduleAll() {
    _alarms.clear();
    const auto& reminders = ReminderStore::getAll();
    for (const auto& r : reminders) {
        scheduleReminder(r);
    }
    time_t best = 0;
    for (const auto& a : _alarms) {
        if (best == 0 || a.trigger_epoch < best) best = a.trigger_epoch;
    }
    if (best > 0) {
        hal::rtc_set_alarm(best);
        time_t now = time(nullptr);
        uint64_t us = (uint64_t)(best - now) * 1000000ULL;
        if (us > 0) {
            esp_sleep_enable_timer_wakeup(us);
            Serial.printf("[SCHEDULER] Sleep timer set: %llu us\n", us);
        }
    } else {
        hal::rtc_clear_alarm();
        Serial.println("[SCHEDULER] No pending reminders");
    }
}

void onWakeup() {
    // FIX-ROUGE-3 : après deep sleep, _alarms est vide (RAM perdue).
    // On recharge le store depuis FATFS et on reconstruit le vecteur
    // AVANT de chercher quel rappel a déclenché le réveil.
    ReminderStore::load();   // re-lit /reminders.json
    _alarms.clear();
    const auto& reminders = ReminderStore::getAll();
    for (const auto& r : reminders) {
        if (!r.enabled) continue;
        time_t trigger = r.datetime - (time_t)(r.advance_minutes * 60);
        _alarms.push_back({r.id, trigger});
    }

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause != ESP_SLEEP_WAKEUP_TIMER && cause != ESP_SLEEP_WAKEUP_EXT0) {
        return;
    }

    time_t now = time(nullptr);
    // Identifier le rappel déclencheur (±10s)
    for (const auto& a : _alarms) {
        if (llabs((long long)(a.trigger_epoch - now)) <= 10) {
            const ReminderStore::Reminder* r = ReminderStore::getById(a.reminder_id);
            if (!r) continue;
            Serial.printf("[SCHEDULER] Wake for reminder %d: %s\n",
                r->id, r->label.c_str());
            // TTS → PCM → DAC (WiFi déjà init dans main.cpp avant cet appel)
            auto pcm = HttpClient::textToSpeech(r->label);
            if (!pcm.empty()) {
                hal::audio_play_pcm(pcm.data(), pcm.size());
            }
        }
    }
    // Replanifier le prochain rappel et reprogrammer le timer deep sleep
    rescheduleAll();
}

void cancelAlarm(int reminder_id) {
    for (auto it = _alarms.begin(); it != _alarms.end(); ++it) {
        if (it->reminder_id == reminder_id) {
            _alarms.erase(it);
            Serial.printf("[SCHEDULER] Cancelled alarm for reminder %d\n", reminder_id);
            rescheduleAll();
            return;
        }
    }
}

} // namespace Scheduler
