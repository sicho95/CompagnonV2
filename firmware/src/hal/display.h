#pragma once
// =============================================================
// CompagnonV2 — hal/display.h
// Driver : CO5300 AMOLED 466×466 via QSPI (Arduino_GFX)
// Lib    : Arduino_GFX_Library (lovyan / moononournation)
// =============================================================
#include <Arduino_GFX_Library.h>
#include "../config/pins.h"

// Instance globale, accessible depuis les autres modules
extern Arduino_CO5300 *gfx;

/**
 * @brief Initialise le bus QSPI et le display.
 *        Applique le registre 0x36=0xA0 (orientation paysage interne).
 *        Doit être appelé AVANT lv_init().
 */
void display_init();

/**
 * @brief Callback LVGL 8 — flush le buffer vers l'écran.
 *        Branche sur Arduino_GFX draw16bitRGBBitmap / BeRGB selon
 *        LV_COLOR_16_SWAP.
 */
void display_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);

/**
 * @brief Callback LVGL 8 — force les coordonnées à être paires
 *        (requis par le CO5300).
 */
void display_rounder_cb(struct _lv_disp_drv_t *disp_drv, lv_area_t *area);
