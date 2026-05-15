#pragma once
// ============================================================
// CompagnonV2 — HAL Display
// Driver : RM67162 QSPI (Waveshare AMOLED 2.16" 480×480)
// Bus    : QSPI 4-bit (SPI quadrature)
// ============================================================
#include <Arduino.h>
#include <lvgl.h>
#include "../config/pins.h"
#include "../config/ui_config.h"

// Taille du draw buffer LVGL (1/10ème de l'écran en PSRAM)
#define DISP_BUF_LINES   48   // lignes par buffer
#define DISP_BUF_SIZE    (SCREEN_W * DISP_BUF_LINES)

/**
 * @brief Initialise le contrôleur RM67162 et enregistre le display LVGL.
 *        À appeler une seule fois dans setup() AVANT lv_init().
 *        Le flush callback QSPI DMA est configuré ici.
 */
void hal_display_init(void);

/**
 * @brief Allume l'écran (backlight + DDIC ON).
 */
void hal_display_on(void);

/**
 * @brief Éteint l'écran (backlight + DDIC sleep) pour économiser la batterie.
 */
void hal_display_off(void);

/**
 * @brief Règle la luminosité (0–255).
 *        Stockée en NVS par le power manager.
 */
void hal_display_set_brightness(uint8_t brightness);

/**
 * @brief Retourne true si l'écran est actuellement allumé.
 */
bool hal_display_is_on(void);
