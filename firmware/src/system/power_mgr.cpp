#include "power_mgr.h"
#include "../hal/pmu.h"
#include "../hal/display.h"
#include <esp_sleep.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "POWER";

static power_state_t s_state       = POWER_STATE_AWAKE;
static uint32_t      s_last_active = 0;  // millis()

void power_mgr_init(void) {
    s_last_active = millis();
    s_state       = POWER_STATE_AWAKE;
    ESP_LOGI(TAG, "Power manager init OK (idle timeout=%ds)", POWER_IDLE_TIMEOUT_S);
}

void power_mgr_reset_idle(void) {
    s_last_active = millis();
    if (s_state != POWER_STATE_AWAKE) {
        s_state = POWER_STATE_AWAKE;
        // TODO : rallumer l'écran via hal_display_power(true)
        ESP_LOGI(TAG, "Réveil — retour AWAKE");
    }
}

void power_mgr_tick(void) {
    if (s_state != POWER_STATE_AWAKE) return;

    uint32_t now    = millis();
    uint32_t idle_s = (now - s_last_active) / 1000;

    // Deep sleep si batterie critique et non en charge
    int bat = hal_pmu_battery_pct();
    if (bat >= 0 && bat < POWER_LOW_BAT_PCT && !hal_pmu_is_charging()) {
        ESP_LOGW(TAG, "Batterie critique (%d%%) — deep sleep", bat);
        s_state = POWER_STATE_DEEP_SLEEP;
        // esp_deep_sleep_start(); // décommenter quand prêt
        return;
    }

    // Light sleep après inactivité
    if (idle_s >= POWER_IDLE_TIMEOUT_S) {
        ESP_LOGI(TAG, "Idle %ds — light sleep", idle_s);
        s_state = POWER_STATE_LIGHT_SLEEP;
        // TODO : hal_display_power(false), suspendre WiFi
        // esp_light_sleep_start(); // décommenter quand wake word ESP-SR configuré
    }
}

void power_mgr_request_sleep(void) {
    ESP_LOGI(TAG, "Light sleep demandé");
    s_state = POWER_STATE_LIGHT_SLEEP;
}

power_state_t power_mgr_get_state(void) { return s_state; }
