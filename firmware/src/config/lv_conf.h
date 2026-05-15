/**
 * CompagnonV2 — lv_conf.h
 * LVGL 9.x — ESP32-S3 + Waveshare AMOLED 2.16" 466×466 (LVGL virtuel 480×480)
 * Arduino framework 3.3.8
 *
 * Placé dans firmware/src/config/ et inclus via -DLV_CONF_INCLUDE_SIMPLE
 * + -I$PROJECT_SRC_DIR/config dans platformio.ini.
 *
 * NOTE API LVGL 9 (différences majeures vs v8) :
 *   - lv_display_create() remplace lv_disp_drv_register()
 *   - lv_indev_create()  remplace lv_indev_drv_register()
 *   - lv_draw_buf_t      remplace lv_disp_draw_buf_t
 *   - lv_display_set_render_mode() remplace LV_DISP_RENDER_MODE_*
 *   - Plus de LV_HOR_RES_MAX / LV_VER_RES_MAX (résolution par display)
 */

#if 1   /* Mettre à 0 pour désactiver ce fichier */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* ─── Profondeur de couleur ─────────────────────────────────────────────────
 * 16 = RGB565 : meilleure perf DMA, suffisant pour AMOLED
 */
#define LV_COLOR_DEPTH 16

/* ─── Endianness RGB565 ─────────────────────────────────────────────────── */
#define LV_BIG_ENDIAN_SYSTEM 0

/* ─── Mémoire LVGL — allocation en PSRAM ────────────────────────────────────
 * LV_USE_STDLIB_MALLOC : 0 = custom malloc
 * ps_malloc / ps_realloc = PSRAM Arduino ESP32 (8 MB disponibles)
 */
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_CUSTOM
#define LV_STDLIB_INCLUDE       <stdlib.h>
#define LV_MALLOC               ps_malloc
#define LV_REALLOC              ps_realloc
#define LV_FREE                 free

/* ─── Taille pool interne (utilisé si LV_USE_STDLIB_MALLOC != LV_STDLIB_CUSTOM) */
#define LV_MEM_SIZE (512U * 1024U)

/* ─── HAL Tick ──────────────────────────────────────────────────────────────
 * Tick géré manuellement dans task_ui_lvgl via lv_tick_inc()
 */
#define LV_TICK_CUSTOM 0

/* ─── Période de rafraîchissement par défaut (ms) ───────────────────────── */
#define LV_DEF_REFR_PERIOD 5

/* ─── DPI ───────────────────────────────────────────────────────────────── */
#define LV_DPI_DEF 130

/* ─── Logging ───────────────────────────────────────────────────────────── */
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

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
#define LV_USE_ARC          1
#define LV_USE_BAR          1   /* jauge batterie */
#define LV_USE_BTN          1
#define LV_USE_BTNMATRIX    1
#define LV_USE_CALENDAR     1
#define LV_USE_CANVAS       1
#define LV_USE_CHART        1
#define LV_USE_CHECKBOX     1
#define LV_USE_DROPDOWN     1
#define LV_USE_IMAGE        1   /* LV_USE_IMG renommé en LV_USE_IMAGE en v9 */
#define LV_USE_IMAGEBUTTON  1
#define LV_USE_KEYBOARD     1   /* clavier tactile — Rappels + Nestor */
#define LV_USE_LABEL        1
#define LV_USE_LINE         1
#define LV_USE_LIST         1
#define LV_USE_MSGBOX       1
#define LV_USE_ROLLER       1
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

/* ─── Thème par défaut ───────────────────────────────────────────────────── */
#define LV_USE_THEME_DEFAULT    1
#define LV_THEME_DEFAULT_DARK   1   /* thème sombre pour AMOLED */

/* ─── Animations ─────────────────────────────────────────────────────────── */
#define LV_USE_ANIM     1

/* ─── Dessin SW ───────────────────────────────────────────────────────────── */
#define LV_USE_DRAW_SW  1

/* ─── FS (activé quand HAL FATFS prêt) ──────────────────────────────────── */
#define LV_USE_FS_FATFS 0

/* ─── Divers ─────────────────────────────────────────────────────────────── */
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR  0

#endif /* LV_CONF_H */
#endif /* enable content */
