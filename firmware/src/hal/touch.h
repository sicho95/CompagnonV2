#pragma once
// =============================================================
// CompagnonV2 — hal/touch.h
// Driver : CST9220 capacitif I2C (lib SensorLib de Lewis He)
// Carte  : Waveshare ESP32-S3-Touch-AMOLED-2.16"
// API    : LVGL 9 — lv_indev_read_cb_t
// =============================================================
#include <Wire.h>
#include <SensorLib.h>
#include <lvgl.h>
#include "../config/pins.h"

extern SensorCST9220 touch;

/**
 * @brief Initialise le CST9220.
 *        Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL) doit avoir été appelé avant.
 * @return true si le capteur répond, false sinon.
 */
bool touch_init();

/**
 * @brief Callback LVGL 9 — lit la position et l'état du touch.
 *        Enregistré via lv_indev_set_read_cb(indev, touch_read_cb)
 */
void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data);
