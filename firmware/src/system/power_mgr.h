#pragma once
/*
 * system/power_mgr.h — Gestionnaire de puissance CompagnonV2
 *
 * Gère la transition entre les modes :
 *   AWAKE      : plein fonctionnement, LVGL actif
 *   LIGHT_SLEEP: écran éteint, WiFi suspendu, Core 1 idle, wake word actif (Core 0)
 *   DEEP_SLEEP : tout arrêté sauf RTC, réveil sur timer (rappel) ou GPIO (bouton)
 *
 * Règles :
 *   - Light sleep après IDLE_TIMEOUT_S secondes sans interaction
 *   - Deep sleep si batterie < LOW_BAT_PCT et pas en charge
 *   - Réveil light sleep → wake word, touch, GPIO bouton, timer rappel
 *   - En light sleep, la tâche voice_task (Core 0) reste active via RTC clock
 */

#ifdef __cplusplus
extern "C" {
#endif

#define POWER_IDLE_TIMEOUT_S   60    // secondes avant light sleep
#define POWER_LOW_BAT_PCT       5    // % batterie pour deep sleep forcé

void power_mgr_init(void);
void power_mgr_tick(void);           // à appeler dans loop()
void power_mgr_reset_idle(void);     // appeler sur toute interaction user
void power_mgr_request_sleep(void);  // forcer light sleep

typedef enum {
    POWER_STATE_AWAKE       = 0,
    POWER_STATE_LIGHT_SLEEP = 1,
    POWER_STATE_DEEP_SLEEP  = 2,
} power_state_t;

power_state_t power_mgr_get_state(void);

#ifdef __cplusplus
}
#endif
