#pragma once
// =============================================================
// CompagnonV2 — hal/touch.h
// Driver : CST816S capacitif I2C (lib SensorLib de Lewis He)
// =============================================================
#include <Wire.h>
#include <SensorLib.h>
#include "../config/pins.h"

extern SensorCST816S touch;

/**
 * @brief Initialise le CST816S.
 *        Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL) doit avoir été appelé avant.
 * @return true si le capteur répond, false sinon.
 */
bool touch_init();

/**
 * @brief Callback LVGL 8 — lit la position et l'état du touch.
 */
void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data);
