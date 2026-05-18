// ============================================================
// CompagnonV2 — system/scheduler.cpp
// B3 — #include "scheduler.h" ajouté
// R6 — rtc_set_alarm() appelé UNE seule fois (PCF85063 = 1 alarme)
// fix: const Reminder* (getById retourne const)
// ============================================================
#include "scheduler.h"
#include "../hal/rtc.h"
#include "../net/http_client.h"
#include <esp_sleep.h>
#include <esp_log.h>
#include <algorithm>

static const char* TAG = "Scheduler";
static std::vector<ScheduledAlarm> _alarms;

namespace os { void kernel_post_alarm(const char* label); }

static ScheduledAlarm* _findAlarm(int reminder_id) {
    for (auto& a : _alarms)
        if (a.reminder_id == reminder_id) return &a;
    return nullptr;
}

static void _updateRtcAlarm() {
    if (_alarms.empty()) {
        hal::rtc_clear_alarm();
        return;
    }
    time_t minEpoch = _alarms[0].trigger_epoch;
    for (const auto& a : _alarms)
        if (a.trigger_epoch < minEpoch) minEpoch = a.trigger_epoch;
    hal::rtc_set_alarm(minEpoch);
    ESP_LOGI(TAG, "RTC alarm set to epoch %lld (next of %d alarms)",
             (long long)minEpoch, (int)_alarms.size());
}

namespace Scheduler {

bool scheduleReminder(const Reminder& r) {
    if (!r.enabled) return false;
    time_t now     = time(nullptr);
    time_t trigger = r.datetime - (time_t)(r.advance_minutes * 60);
    if (trigger <= now) {
        ESP_LOGW(TAG, "Reminder %d trigger in the past, skipping", r.id);
        return false;
    }
    cancelAlarm(r.id);
    ScheduledAlarm a;
    a.reminder_id   = r.id;
    a.trigger_epoch = trigger;
    _alarms.push_back(a);
    ESP_LOGI(TAG, "Queued reminder %d at epoch %lld", r.id, (long long)trigger);
    _updateRtcAlarm();
    return true;
}

void rescheduleAll() {
    _alarms.clear();
    hal::rtc_clear_alarm();
    for (const auto& r : ReminderStore::getAll()) {
        if (!r.enabled) continue;
        time_t now     = time(nullptr);
        time_t trigger = r.datetime - (time_t)(r.advance_minutes * 60);
        if (trigger > now) {
            ScheduledAlarm a;
            a.reminder_id   = r.id;
            a.trigger_epoch = trigger;
            _alarms.push_back(a);
        }
    }
    _updateRtcAlarm();
    if (!_alarms.empty()) {
        time_t now  = time(nullptr);
        time_t next = _alarms[0].trigger_epoch;
        for (const auto& a : _alarms)
            if (a.trigger_epoch < next) next = a.trigger_epoch;
        int64_t delta = (int64_t)(next - now);
        if (delta > 0) {
            esp_sleep_enable_timer_wakeup((uint64_t)delta * 1000000ULL);
            ESP_LOGI(TAG, "Deep sleep ready for %lld s", (long long)delta);
        }
    }
    ESP_LOGI(TAG, "rescheduleAll: %d active alarms", (int)_alarms.size());
}

void onWakeup() {
    time_t now = time(nullptr);
    for (const auto& a : _alarms) {
        if (a.trigger_epoch <= now + 5) {
            const Reminder* r = ReminderStore::getById(a.reminder_id);
            if (r && r->enabled) {
                ESP_LOGI(TAG, "Wakeup: reminder %d (%s)", r->id, r->label.c_str());
                os::kernel_post_alarm(r->label.c_str());
            }
        }
    }
}

void cancelAlarm(int reminder_id) {
    auto it = std::remove_if(_alarms.begin(), _alarms.end(),
                             [reminder_id](const ScheduledAlarm& a) {
                                 return a.reminder_id == reminder_id;
                             });
    if (it != _alarms.end()) {
        _alarms.erase(it, _alarms.end());
        ESP_LOGI(TAG, "Cancelled alarm for reminder %d", reminder_id);
        _updateRtcAlarm();
    }
}

const std::vector<ScheduledAlarm>& getScheduled() {
    return _alarms;
}

} // namespace Scheduler
