// ============================================================
// CompagnonV2 — system/scheduler.h
// Planification rappels → alarme RTC + deep sleep timer
// ============================================================
#pragma once
#include <time.h>
#include "../storage/reminder_store.h"

namespace Scheduler {

// Planifie un rappel individuel
void scheduleReminder(const ReminderStore::Reminder& r);

// (Re)planifie TOUS les rappels actifs — appeler après boot NTP
void rescheduleAll();

// Handler wakeup — appeler si esp_sleep_get_wakeup_cause() == timer/ext
void onWakeup();

// Annule une alarme planifiée
void cancelAlarm(int reminder_id);

} // namespace Scheduler
