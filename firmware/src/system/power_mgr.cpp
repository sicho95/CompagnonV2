// ============================================================
// CompagnonV2 — system/power_mgr.cpp
// DEBUG : sleep désactivé (SLEEP_DISABLED 1)
// Remettre à 0 pour le mode production.
// ============================================================
#include "power_mgr.h"
#include <Arduino.h>
#include <esp_sleep.h>

#define SLEEP_DISABLED   1      // <- passer à 0 en production
#define INACTIVITY_MS    600000UL
#define BOOT_GRACE_MS    60000UL

static unsigned long _last_activity   = 0;
static bool          _sleep_requested = false;

void power_mgr_init()  { _last_activity = millis(); }
void power_mgr_activity() { _last_activity = millis(); }
void power_mgr_request_sleep() { _sleep_requested = true; }

void power_mgr_tick() {
#if SLEEP_DISABLED
    return;  // sleep totalement désactivé en debug
#endif
    unsigned long now = millis();
    if (now < BOOT_GRACE_MS) return;
    if (_sleep_requested || (now - _last_activity > INACTIVITY_MS)) {
        Serial.println("[PWR] light sleep");
        esp_sleep_enable_timer_wakeup(10ULL * 1000000ULL);
        esp_light_sleep_start();
        _last_activity   = millis();
        _sleep_requested = false;
    }
}
