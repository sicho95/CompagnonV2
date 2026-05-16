#pragma once
// =============================================================
// CompagnonV2 — hal/pmu.h
// Driver : AXP2101 via XPowersLib 0.3.x
// IRQ activées : PKEY_SHORT (backlight toggle)
//                PKEY_LONG  (poweroff complet)
// =============================================================
#include <Wire.h>
#include <XPowersLib.h>
#include "../../include/pins.h"

// XPowersLib 0.3.x : la classe s'appelle XPowersAXP2101, pas XPowersPMU
extern XPowersAXP2101 pmu;

// Événements PMU remontés à la tâche OS
typedef enum {
    PMU_EVT_NONE      = 0,
    PMU_EVT_PWR_SHORT,   // appui court PWR → toggle backlight
    PMU_EVT_PWR_LONG,    // appui long  PWR → arrêt complet
} pmu_event_t;

// Callback long press (utilisé par le .ino)
typedef void (*pmu_long_press_cb_t)();

bool         pmu_init();
pmu_event_t  pmu_handle_irq();
void         pmu_poweroff();
int          pmu_battery_percent();
bool         pmu_is_charging();
uint16_t     pmu_battery_voltage_mv();
void         pmu_set_long_press_cb(pmu_long_press_cb_t cb);

extern volatile bool pmu_irq_flag;

// ── Aliases flat C pour le .ino ──────────────────────────────────
inline bool hal_pmu_init()                           { return pmu_init(); }
inline void hal_pmu_tick()                           { pmu_handle_irq(); }
inline int  hal_pmu_battery_pct()                    { return pmu_battery_percent(); }
inline bool hal_pmu_is_charging()                    { return pmu_is_charging(); }
inline void hal_pmu_set_long_press_cb(pmu_long_press_cb_t cb) { pmu_set_long_press_cb(cb); }
