#pragma once
// ============================================================
// CompagnonV2 — hal/display.h
// Pilote écran rm67162 QSPI 480×480
// Waveshare ESP32-S3 AMOLED 2.16"
// Arduino 3.3.8 + LVGL 8.4.x
// ============================================================
#include <Arduino.h>
#include <lvgl.h>
#include "../config/pins.h"
#include "../config/ui_config.h"

// Taille des buffers DMA LVGL (en pixels)
// 1/10 de la zone logique en double-buffering PSRAM
#define DISP_BUF_LINES   46   // APP_H / 10
#define DISP_BUF_SIZE    (SCREEN_W * DISP_BUF_LINES)

bool     display_init();
void     display_flush_cb(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p);
uint16_t display_get_brightness();
void     display_set_brightness(uint16_t brightness);  // 0-255
void     display_sleep();
void     display_wake();
