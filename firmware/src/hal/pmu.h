#pragma once
// ============================================================
// CompagnonV2 — HAL PMU
// Chip : AXP2101 (via XPowersLib)
// I2C  : partagé IIC_SDA/SCL (GPIO 15/14)
// IRQ  : AXP_INT GPIO 13 (FALLING, IRAM_ATTR)
// ============================================================
#include <Arduino.h>
#include <XPowersLib.h>
#include "../config/pins.h"

// Seuils batterie pour couleur UI
#define PMU_BAT_LOW      15   // % — rouge
#define PMU_BAT_WARN     30   // % — orange (en dessous)
#define PMU_BAT_OK       30   // % — vert (au dessus)

typedef enum {
    PMU_BAT_COLOR_GREEN,
    PMU_BAT_COLOR_ORANGE,
    PMU_BAT_COLOR_RED
} pmu_bat_color_t;

typedef struct {
    bool            charging;        // true si en charge USB
    bool            usb_present;     // câble USB branché
    uint8_t         battery_pct;     // 0–100 %
    uint16_t        voltage_mv;      // tension batterie en mV
    pmu_bat_color_t color;           // couleur pour la status bar
} pmu_status_t;

/**
 * @brief Initialise l'AXP2101 : rails de puissance, IRQ, lecture initiale.
 */
bool hal_pmu_init(void);

/**
 * @brief Met à jour l'état PMU (à appeler dans task_os_main toutes les ~1s).
 */
void hal_pmu_tick(void);

/**
 * @brief Retourne le dernier état connu du PMU.
 */
const pmu_status_t* hal_pmu_get_status(void);

/**
 * @brief Éteint le device (power off complet).
 */
void hal_pmu_power_off(void);
