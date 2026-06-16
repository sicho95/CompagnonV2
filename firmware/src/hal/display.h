// ============================================================
// CompagnonV2 — hal/display.h
// ============================================================
#pragma once
#include <lvgl.h>
#include <cstdint>

#define LCD_WIDTH_PHYS  480
#define LCD_HEIGHT_PHYS 480
#define LCD_WIDTH       480
#define LCD_HEIGHT      480

namespace hal {
    void                  display_init();
    lv_display_t*         display_get();
    lv_display_rotation_t display_get_rotation();
    void                  display_set_rotation(lv_display_rotation_t rot);
    void                  display_set_brightness(uint8_t pct);
    void                  display_sleep();
    void                  display_wakeup();
    void                  display_force_refresh();
}
