#pragma once
// =============================================================
// CompagnonV2 — hal/pmu.h
// Driver : AXP2101 via XPowersLib
// IRQ activées : PKEY_SHORT (backlight toggle)
//                PKEY_LONG  (poweroff complet)
// =============================================================
#include <Wire.h>
#include <XPowersLib.h>
#include "../config/pins.h"

extern XPowersPMU pmu;

// Événements PMU remontés à task_os_main
typedef enum {
    PMU_EVT_NONE      = 0,
    PMU_EVT_PWR_SHORT,   // appui court PWR → toggle backlight
    PMU_EVT_PWR_LONG,    // appui long  PWR → arrêt complet
} pmu_event_t;

/**
 * @brief Initialise l'AXP2101.
 *        Active ADC batterie/VBUS/système.
 *        Active PKEY_SHORT_IRQ + PKEY_LONG_IRQ.
 * @return true si PMU répond.
 */
bool pmu_init();

/**
 * @brief À appeler dans la tâche OS (polling ou depuis ISR).
 *        Lit les IRQ, remet le flag à zéro.
 * @return L'événement détecté (PMU_EVT_NONE si rien).
 */
pmu_event_t pmu_handle_irq();

/** @brief Éteint complètement l'appareil via AXP2101. */
void pmu_poweroff();

/** @brief Retourne le % batterie (0–100), -1 si pas de batterie. */
int  pmu_battery_percent();

/** @brief true si en charge. */
bool pmu_is_charging();

/** @brief Tension batterie en mV. */
uint16_t pmu_battery_voltage_mv();

/** @brief Flag IRQ levé depuis ISR → lu par pmu_handle_irq(). */
extern volatile bool pmu_irq_flag;
