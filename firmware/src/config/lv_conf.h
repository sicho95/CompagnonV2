/**
 * CompagnonV2 — lv_conf.h
 * LVGL 9.3.0 — ESP32-S3 + Waveshare AMOLED 2.16" 480×480
 * Arduino framework 3.3.8
 *
 * Pour LVGL 9.x la structure de lv_conf.h a changé par rapport à LVGL 8.
 * Ce fichier est le template officiel LVGL 9 adapté pour CompagnonV2.
 */

#if 1 /* Mettre à 0 pour désactiver ce fichier */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* ─── Profondeur de couleur ────────────────────────────────────────────── */
/* 16 = RGB565 (suffisant pour AMOLED, meilleur perf DMA) */
#define LV_COLOR_DEPTH 16

/* ─── Mémoire LVGL — PSRAM custom alloc ────────────────────────────────── */
/* LVGL 9 : on utilise toujours LV_MEM_CUSTOM=1 avec ps_malloc */
#define LV_MEM_CUSTOM 1
#if LV_MEM_CUSTOM
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
    #define LV_MEM_CUSTOM_ALLOC   ps_malloc
    #define LV_MEM_CUSTOM_FREE    free
    #define LV_MEM_CUSTOM_REALLOC ps_realloc
#endif

/* ─── Tick (géré par task_ui_lvgl via lv_tick_inc) ─────────────────────── */
/* On utilise lv_tick_inc() appelé depuis FreeRTOS, pas de custom tick expr */
#define LV_TICK_CUSTOM 0

/* ─── Logging ───────────────────────────────────────────────────────────── */
#define LV_USE_LOG      1
#define LV_LOG_LEVEL    LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF   1  /* utilise printf → Serial en Arduino */

/* ─── Asserts ───────────────────────────────────────────────────────────── */
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0
#define LV_USE_ASSERT_STYLE         0

/* ─── Polices Montserrat intégrées ──────────────────────────────────────── */
#define LV_FONT_MONTSERRAT_12   1
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_16   1
#define LV_FONT_MONTSERRAT_20   1
#define LV_FONT_MONTSERRAT_24   1
#define LV_FONT_MONTSERRAT_28   0
#define LV_FONT_DEFAULT         &lv_font_montserrat_14

/* ─── Widgets (LVGL 9 — tous les noms LV_USE_* sont identiques à v8) ───── */
#define LV_USE_ANIMIMG      0
#define LV_USE_ARC          1
#define LV_USE_BAR          1
#define LV_USE_BTN          1
#define LV_USE_BTNMATRIX    1
#define LV_USE_CALENDAR     1
#define LV_USE_CANVAS       1
#define LV_USE_CHART        1
#define LV_USE_CHECKBOX     1
#define LV_USE_DROPDOWN     1
#define LV_USE_IMG          1
#define LV_USE_IMGBTN       1
#define LV_USE_KEYBOARD     1   /* clavier tactile — important pour Rappels */
#define LV_USE_LABEL        1
#define LV_USE_LED          0
#define LV_USE_LINE         1
#define LV_USE_LIST         1
#define LV_USE_MENU         1
#define LV_USE_METER        1   /* jauge batterie */
#define LV_USE_MSGBOX       1
#define LV_USE_ROLLER       1
#define LV_USE_SCALE        1   /* LVGL 9 : remplace lv_meter partiellement */
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

/* ─── Thème ─────────────────────────────────────────────────────────────── */
#define LV_USE_THEME_DEFAULT    1
#define LV_THEME_DEFAULT_DARK   1   /* thème sombre pour AMOLED */

/* ─── Animations ────────────────────────────────────────────────────────── */
/* LVGL 9 : les animations sont intégrées, pas de define séparé */

/* ─── Encoders / input devices ──────────────────────────────────────────── */
#define LV_USE_INDEV_ENCODER    0
#define LV_USE_INDEV_KEYPAD     0

/* ─── FS (optionnel — pour chargement images depuis FATFS) ──────────────── */
#define LV_USE_FS_FATFS         0   /* activé plus tard quand HAL FATFS prêt */

/* ─── GPU / DMA2D ───────────────────────────────────────────────────────── */
/* ESP32-S3 n'a pas de GPU — on n'active rien */

#endif /* LV_CONF_H */
#endif /* enable content */
