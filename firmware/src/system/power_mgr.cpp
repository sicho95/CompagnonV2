#include "power_mgr.h"
#include <Arduino.h>
#include <esp_sleep.h>

// Inactivité avant sleep : 10 min (600 000 ms)
// Augmenté depuis 2 min pour laisser le temps à l'UI de s'afficher au boot
#define INACTIVITY_MS  600000UL

// Délai de grâce au démarrage : 60 s avant d'activer la surveillance d'inactivité
// Permet à LVGL de flusher la première frame sans être interrompu par le sleep
#define BOOT_GRACE_MS  60000UL

static unsigned long _last_activity   = 0;
static bool          _sleep_requested = false;

void power_mgr_init()  { _last_activity = millis(); }

void power_mgr_activity() { _last_activity = millis(); }

void power_mgr_request_sleep() { _sleep_requested = true; }

void power_mgr_tick() {
    unsigned long now = millis();

    // Grâce au boot : pas de sleep pendant BOOT_GRACE_MS
    if (now < BOOT_GRACE_MS) return;

    if (_sleep_requested || (now - _last_activity > INACTIVITY_MS)) {
        Serial.println("[PWR] light sleep");
        esp_sleep_enable_timer_wakeup(10ULL * 1000000ULL); // réveil 10 s
        esp_light_sleep_start();
        _last_activity   = millis();
        _sleep_requested = false;
    }
}
