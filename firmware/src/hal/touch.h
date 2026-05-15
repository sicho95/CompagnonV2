#pragma once
// ============================================================
// CompagnonV2 — HAL Touch
// Contrôleur : CST816S (I2C)
// I2C partagé avec PMU (AXP2101) et RTC (PCF85063)
// IRQ touch sur GPIO dédié ou partagé selon config
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include "../config/pins.h"

// Adresse I2C du CST816S
#define CST816S_ADDR   0x15

// Seuils
#define TOUCH_MAX_POINTS 1   // CST816S = single touch

typedef struct {
    bool     pressed;   // true si un doigt est posé
    int16_t  x;
    int16_t  y;
    uint8_t  gesture;  // 0=none, 1=swipe_up, 2=swipe_down, 3=swipe_left, 4=swipe_right
} touch_data_t;

/**
 * @brief Initialise le CST816S et enregistre l'indev LVGL touch.
 *        À appeler après hal_display_init().
 */
void hal_touch_init(void);

/**
 * @brief Lit les données de touch brutes (pour debug ou usage hors LVGL).
 */
bool hal_touch_read(touch_data_t* out);

/**
 * @brief Callback indev LVGL (enregistré en interne, ne pas appeler manuellement).
 */
void hal_touch_lvgl_read_cb(lv_indev_t* indev, lv_indev_data_t* data);
