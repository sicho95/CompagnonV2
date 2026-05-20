#pragma once
// =============================================================
// CompagnonV2 — hal/pmu.h
// Driver : AXP2101 via XPowersLib 0.3.x
// fix: supprime extern XPowersAXP2101 pmu (global dangereux)
//      remplace par hal::pmu_get() — retourne nullptr avant pmu_init()
// =============================================================
#include <Wire.h>
#include <XPowersLib.h>
#include "../../include/pins.h"

typedef enum {
    PMU_EVT_NONE      = 0,
    PMU_EVT_PWR_SHORT,
    PMU_EVT_PWR_LONG,
} pmu_event_t;

typedef void (*pmu_long_press_cb_t)();

extern volatile bool pmu_irq_flag;

namespace hal {
    bool              pmu_init();
    pmu_event_t       pmu_handle_irq();
    void              pmu_poweroff();
    int               pmu_battery_percent();
    bool              pmu_is_charging();
    uint16_t          pmu_battery_voltage_mv();
    void              pmu_set_long_press_cb(pmu_long_press_cb_t cb);
    XPowersAXP2101*   pmu_get();   // nullptr avant pmu_init()
} // namespace hal

// Aliases flat C pour le .ino
inline bool hal_pmu_init()                                    { return hal::pmu_init(); }
inline void hal_pmu_tick()                                    { hal::pmu_handle_irq(); }
inline int  hal_pmu_battery_pct()                             { return hal::pmu_battery_percent(); }
inline bool hal_pmu_is_charging()                             { return hal::pmu_is_charging(); }
inline void hal_pmu_set_long_press_cb(pmu_long_press_cb_t cb) { hal::pmu_set_long_press_cb(cb); }
