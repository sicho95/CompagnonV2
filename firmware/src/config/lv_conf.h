/**
 * CompagnonV2 — LVGL configuration
 * Target: ESP32-S3 + Waveshare AMOLED 2.16" (480x480)
 */
#if 1 /* Set to 1 to enable content below */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* Color depth: 16 (RGB565) or 32 (ARGB8888) */
#define LV_COLOR_DEPTH 16

/* Display resolution */
#define LV_HOR_RES_MAX 480
#define LV_VER_RES_MAX 480

/* Memory for LVGL — use PSRAM if available */
#define LV_MEM_CUSTOM 1
#if LV_MEM_CUSTOM
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
    #define LV_MEM_CUSTOM_ALLOC   ps_malloc  /* use PSRAM */
    #define LV_MEM_CUSTOM_FREE    free
    #define LV_MEM_CUSTOM_REALLOC ps_realloc
#else
    #define LV_MEM_SIZE (128U * 1024U)  /* 128kB from internal SRAM */
#endif

/* Tick interface (handled by FreeRTOS task) */
#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "esp_timer.h"
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (esp_timer_get_time() / 1000)
#endif

/* Font support */
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_DEFAULT       &lv_font_montserrat_14

/* Enable extra widgets */
#define LV_USE_ARC         1
#define LV_USE_BAR         1
#define LV_USE_BTN         1
#define LV_USE_BTNMATRIX   1
#define LV_USE_CANVAS      1
#define LV_USE_CHART       1
#define LV_USE_CHECKBOX    1
#define LV_USE_DROPDOWN    1
#define LV_USE_IMG         1
#define LV_USE_IMGBTN      1
#define LV_USE_KEYBOARD    1
#define LV_USE_LABEL       1
#define LV_USE_LED         1
#define LV_USE_LINE        1
#define LV_USE_LIST        1
#define LV_USE_METER       1
#define LV_USE_MSGBOX      1
#define LV_USE_ROLLER      1
#define LV_USE_SLIDER      1
#define LV_USE_SPAN        1
#define LV_USE_SPINBOX     1
#define LV_USE_SPINNER     1
#define LV_USE_SWITCH      1
#define LV_USE_TABLE       1
#define LV_USE_TABVIEW     1
#define LV_USE_TEXTAREA    1
#define LV_USE_TILEVIEW    1
#define LV_USE_WIN         1

/* Animations */
#define LV_USE_ANIMATION  1

/* Log level */
#define LV_USE_LOG        1
#define LV_LOG_LEVEL      LV_LOG_LEVEL_WARN

/* Debug */
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_MEM_INTEGRITY 0

#endif /* LV_CONF_H */
#endif /* Enable content */
