/**
 * CompagnonV2 — lv_conf.h
 * LVGL 9.x — ESP32-S3 + Waveshare AMOLED 2.16" 466×466 (LVGL 480×480 virtuel)
 * Arduino framework 3.3.8 / PlatformIO
 *
 * Inclus via -DLV_CONF_INCLUDE_SIMPLE + -I$PROJECT_SRC_DIR/config
 */

#if 1  /* Mettre à 0 pour désactiver ce fichier */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* ─── Profondeur de couleur ─────────────────────────────────────
 * 16 = RGB565 : perf DMA optimale pour CO5300 QSPI
 */
#define LV_COLOR_DEPTH 16

/* ─── Résolution par défaut (LVGL 9 : optionnel ici, défini à l'init) ─── */
#define LV_HOR_RES  480
#define LV_VER_RES  480

/* ─── Mémoire — allocation en PSRAM ─────────────────────────────
 * LV_MEM_CUSTOM=1 : fonctions custom ps_malloc / ps_realloc
 * 8 MB PSRAM disponible sur ESP32-S3
 */
#define LV_MEM_CUSTOM 1
#if LV_MEM_CUSTOM
    #define LV_MEM_CUSTOM_INCLUDE  <stdlib.h>
    #define LV_MEM_CUSTOM_ALLOC    ps_malloc
    #define LV_MEM_CUSTOM_FREE     free
    #define LV_MEM_CUSTOM_REALLOC  ps_realloc
#endif

/* ─── Tick ───────────────────────────────────────────────────────
 * Géré manuellement dans task_ui_lvgl via lv_tick_inc(elapsed)
 */
#define LV_TICK_CUSTOM 0

/* ─── Période de rafraîchissement (ms) ──────────────────────────*/
#define LV_DEF_REFR_PERIOD 10   /* ~100 fps max */

/* ─── DPI ────────────────────────────────────────────────────── */
#define LV_DPI_DEF 295   /* 466px / 1.58" ≈ 295 dpi */

/* ─── Logging ────────────────────────────────────────────────── */
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

/* ─── Asserts ────────────────────────────────────────────────── */
#define LV_USE_ASSERT_NULL     1
#define LV_USE_ASSERT_MALLOC   1
#define LV_USE_ASSERT_OBJ      0
#define LV_USE_ASSERT_STYLE    0

/* ─── Polices Montserrat embarquées ─────────────────────────── */
#define LV_FONT_MONTSERRAT_12  1
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_16  1
#define LV_FONT_MONTSERRAT_20  1
#define LV_FONT_MONTSERRAT_24  1
#define LV_FONT_MONTSERRAT_28  0
#define LV_FONT_DEFAULT        &lv_font_montserrat_14

/* ─── Widgets LVGL 9 ─────────────────────────────────────────── */
#define LV_USE_ARC          1
#define LV_USE_BAR          1   /* jauge batterie */
#define LV_USE_BUTTON       1
#define LV_USE_BUTTONMATRIX 1
#define LV_USE_CALENDAR     1
#define LV_USE_CANVAS       1
#define LV_USE_CHART        1
#define LV_USE_CHECKBOX     1
#define LV_USE_DROPDOWN     1
#define LV_USE_IMAGE        1
#define LV_USE_IMAGEBUTTON  1
#define LV_USE_KEYBOARD     1   /* clavier tactile Rappels + Nestor */
#define LV_USE_LABEL        1
#define LV_USE_LINE         1
#define LV_USE_LIST         1
#define LV_USE_MSGBOX       1
#define LV_USE_ROLLER       1
#define LV_USE_SCALE        1   /* remplace LV_USE_METER en v9 */
#define LV_USE_SLIDER       1
#define LV_USE_SPAN         1
#define LV_USE_SPINBOX      1
#define LV_USE_SPINNER      1
#define LV_USE_SWITCH       1
#define LV_USE_TABLE        1
#define LV_USE_TABVIEW      1
#define LV_USE_TEXTAREA     1
#define LV_USE_TILEVIEW     1   /* launcher carousel */
#define LV_USE_WIN          1

/* ─── Thème sombre AMOLED ────────────────────────────────────── */
#define LV_USE_THEME_DEFAULT    1
#define LV_THEME_DEFAULT_DARK   1

/* ─── Animations ─────────────────────────────────────────────── */
#define LV_USE_ANIM    1

/* ─── Draw SW ────────────────────────────────────────────────── */
#define LV_USE_DRAW_SW 1
#define LV_USE_DRAW_SW_COMPLEX_GRADIENTS 0

/* ─── FS (activé quand FATFS HAL prêt) ──────────────────────── */
#define LV_USE_FS_FATFS 0

#endif /* LV_CONF_H */
#endif /* enable content */
