/**
 * lv_conf.h — LVGL 9 config CompagnonV2
 * Ecran : CO5300 AMOLED QSPI, 466x466 (zone active), RGB565
 *
 * IMPORTANT tick :
 *   LV_TICK_CUSTOM 1 → LVGL lit millis() automatiquement.
 *   NE PAS appeler lv_tick_inc() dans loop().
 *
 * IMPORTANT endianness :
 *   LV_COLOR_16_SWAP 0 — le CO5300 via Arduino_GFX attend du Little Endian.
 *   Valide par test fillRect : Rouge/Vert/Bleu/Blanc corrects sans swap.
 */
#ifndef LV_CONF_H
#define LV_CONF_H
#include <stdint.h>

#define LV_COLOR_DEPTH   16
#define LV_COLOR_16_SWAP 0   // CO5300 = Little Endian, pas de swap

#define LV_MEM_CUSTOM         1
#define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
#define LV_MEM_CUSTOM_ALLOC   ps_malloc
#define LV_MEM_CUSTOM_REALLOC ps_realloc
#define LV_MEM_CUSTOM_FREE    free

#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_DEFAULT       &lv_font_montserrat_16

#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE <Arduino.h>
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR  0

#define LV_USE_LABEL    1
#define LV_USE_BTN      1
#define LV_USE_ARC      1
#define LV_USE_BAR      1
#define LV_USE_LIST     1
#define LV_USE_MSGBOX   1
#define LV_USE_TABVIEW  1
#define LV_USE_TILEVIEW 1
#define LV_USE_TEXTAREA 1
#define LV_USE_KEYBOARD 1
#define LV_USE_SPINNER  1
#define LV_USE_ANIMIMG  1
#define LV_USE_IMG      1
#define LV_USE_CHART    0

#define LV_USE_THEME_DEFAULT  1
#define LV_THEME_DEFAULT_DARK 1

#define LV_HOR_RES_MAX 466
#define LV_VER_RES_MAX 466
#define LV_DISPLAY_ROTATION LV_DISPLAY_ROTATION_0

#endif
