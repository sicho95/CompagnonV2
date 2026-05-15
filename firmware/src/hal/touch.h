#pragma once
// ============================================================
// CompagnonV2 — hal/touch.h
// Pilote tactile CST816S I2C
// Waveshare ESP32-S3 AMOLED 2.16"
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#include "../config/pins.h"
#include "../config/ui_config.h"

#define CST816S_ADDR     0x15
#define CST816S_REG_GEST 0x01
#define CST816S_REG_NPTS 0x02
#define CST816S_REG_XH   0x03
#define CST816S_REG_XL   0x04
#define CST816S_REG_YH   0x05
#define CST816S_REG_YL   0x06

bool touch_init();
void touch_read_cb(lv_indev_drv_t* drv, lv_indev_data_t* data);
