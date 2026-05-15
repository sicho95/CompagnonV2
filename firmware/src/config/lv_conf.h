/**
 * CompagnonV2 — lv_conf.h
 * LVGL 8.4.x — ESP32-S3 + Waveshare AMOLED 2.16" 480×480
 * Arduino framework 3.3.8
 *
 * Placé dans firmware/src/config/ et inclus via -I flag PlatformIO
 * grâce au define -DLV_CONF_INCLUDE_SIMPLE dans platformio.ini.
 */

#if 1   /* Mettre à 0 pour désactiver ce fichier */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* ─── Profondeur de couleur ─────────────────────────────────────────────────
 * 16 = RGB565 : meilleure perf DMA, suffisant pour AMOLED
 */
#define LV_COLOR_DEPTH     16
#define LV_COLOR_16_SWAP   0

/* ─── Mémoire LVGL — allocation en PSRAM ────────────────────────────────────
 * LV_MEM_CUSTOM=1 : on fournit nos propres fonctions d'alloc.
 * ps_malloc / ps_realloc = alloc en PSRAM Arduino ESP32.
 * Avec 8 MB PSRAM on peut allouer généreusement.
 */
#define LV_MEM_CUSTOM      1
#if LV_MEM_CUSTOM
    #define LV_MEM_CUSTOM_INCLUDE  <stdlib.h>
    #define LV_MEM_CUSTOM_ALLOC    ps_malloc
    #define LV_MEM_CUSTOM_FREE     free
    #define LV_MEM_CUSTOM_REALLOC  ps_realloc
#endif

/* ─── Taille mémoire interne (si LV_MEM_CUSTOM=0, non utilisé ici) ──────── */
#define LV_MEM_SIZE  (512U * 1024U)   /* 512 KB — référence seulement */

/* ─── HAL Tick ──────────────────────────────────────────────────────────────
 * Tick géré manuellement par task_ui_lvgl via lv_tick_inc(1) toutes les 1ms.
 */
#define LV_TICK_CUSTOM     0

/* ─── Résolution max (pour allocation buffers internes LVGL) ────────────── */
#define LV_HOR_RES_MAX     480
#define LV_VER_RES_MAX     480

/* ─── Logging ───────────────────────────────────────────────────────────── */
#define LV_USE_LOG         1
#define LV_LOG_LEVEL       LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF      1   /* Serial.printf via printf Arduino */

/* ─── Asserts ───────────────────────────────────────────────────────────── */
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0
#define LV_USE_ASSERT_STYLE         0

/* ─── Polices Montserrat embarquées ─────────────────────────────────────── */
#define LV_FONT_MONTSERRAT_12  1
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_16  1
#define LV_FONT_MONTSERRAT_20  1
#define LV_FONT_MONTSERRAT_24  1
#define LV_FONT_MONTSERRAT_28  0
#define LV_FONT_DEFAULT        &lv_font_montserrat_14

/* ─── Widgets ────────────────────────────────────────────────────────────── */
#define LV_USE_ARC         1
#define LV_USE_BAR         1   /* jauge batterie */
#define LV_USE_BTN         1
#define LV_USE_BTNMATRIX   1
#define LV_USE_CALENDAR    1
#define LV_USE_CANVAS      1
#define LV_USE_CHART       1
#define LV_USE_CHECKBOX    1
#define LV_USE_DROPDOWN    1
#define LV_USE_IMG         1
#define LV_USE_IMGBTN      1
#define LV_USE_KEYBOARD    1   /* clavier tactile — Rappels + Nestor */
#define LV_USE_LABEL       1
#define LV_USE_LED         0
#define LV_USE_LINE        1
#define LV_USE_LIST        1
#define LV_USE_METER       1   /* jauge alternative */
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
#define LV_USE_TILEVIEW    1   /* launcher carousel */
#define LV_USE_WIN         1

/* ─── Thème par défaut ───────────────────────────────────────────────────── */
#define LV_USE_THEME_DEFAULT   1
#define LV_THEME_DEFAULT_DARK  1   /* thème sombre pour AMOLED */

/* ─── Animations ─────────────────────────────────────────────────────────── */
#define LV_USE_ANIMATION   1

/* ─── GPU / DMA ──────────────────────────────────────────────────────────── */
/* ESP32-S3 pas de GPU dédié */
#define LV_USE_GPU_STM32_DMA2D  0
#define LV_USE_GPU_NXP_PXP      0

/* ─── FS (activé quand HAL FATFS prêt) ──────────────────────────────────── */
#define LV_USE_FS_FATFS  0

#endif /* LV_CONF_H */
#endif /* enable content */
