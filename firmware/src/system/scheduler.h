#pragma once
#include <Arduino.h>
// Fix 2 : include reminder_store.h pour que Reminder soit connu
// de tous les TU qui incluent scheduler.h sans inclure reminder_store.h séparément.
#include "../storage/reminder_store.h"
#include <vector>

struct ScheduledAlarm {
    int    reminder_id;
    time_t trigger_epoch;  // r.datetime - r.advance_minutes * 60
};

namespace Scheduler {
    // Fix 2 : const Reminder& — plus d'ambiguïté de type
    bool scheduleReminder(const Reminder& r);
    void rescheduleAll();
    void onWakeup();
    void cancelAlarm(int reminder_id);
    const std::vector<ScheduledAlarm>& getScheduled();
}
