#include "power_mgr.h"
#include <Arduino.h>
#include <esp_sleep.h>

#define INACTIVITY_MS  120000UL  // 2 min

static unsigned long _last_activity = 0;
static bool          _sleep_requested = false;

void power_mgr_init() { _last_activity = millis(); }

void power_mgr_request_sleep() { _sleep_requested = true; }

void power_mgr_tick() {
    if (_sleep_requested || (millis() - _last_activity > INACTIVITY_MS)) {
        Serial.println("[PWR] light sleep");
        esp_sleep_enable_timer_wakeup(10ULL * 1000000ULL);
        esp_light_sleep_start();
        _last_activity    = millis();
        _sleep_requested  = false;
    }
}
