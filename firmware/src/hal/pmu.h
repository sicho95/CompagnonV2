#pragma once
// =============================================================
// CompagnonV2 — hal/pmu.h
// Driver : AXP2101 via XPowersLib
// =============================================================
#include <Wire.h>
#include <XPowersLib.h>
#include "../config/pins.h"

extern XPowersPMU pmu;

/**
 * @brief Initialise l'AXP2101.
 *        Active les ADC batterie/VBUS/système.
 *        Configure la cible de charge à 4.2V.
 *        Active l'IRQ PKEY_SHORT pour le bouton power.
 * @return true si PMU répond, false sinon.
 */
bool pmu_init();

/** @brief Traitement des IRQ PMU (à appeler depuis la tâche OS). */
void pmu_handle_irq();

/** @brief Retourne le pourcentage batterie (0–100), -1 si pas de batterie. */
int  pmu_battery_percent();

/** @brief Retourne true si en charge. */
bool pmu_is_charging();

/** @brief Retourne la tension batterie en mV. */
uint16_t pmu_battery_voltage_mv();

/** @brief Flag IRQ levé depuis l'ISR → lu et réinitialisé par pmu_handle_irq(). */
extern volatile bool pmu_irq_flag;
