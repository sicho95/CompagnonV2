// ============================================================
// CompagnonV2 — hal/display.h
// LCD_WIDTH / LCD_HEIGHT / LCD_WIDTH_PHYS / LCD_HEIGHT_PHYS
// sont définis dans include/pins.h — source de vérité unique.
// Ne pas les redéfinir ici.
// ============================================================
#pragma once
#include <lvgl.h>
#include <cstdint>

// pins.h est la source de vérité pour LCD_WIDTH/HEIGHT
// (inclus transitivement via pmu.h ou directement)

namespace hal {
    void                  display_init();
    lv_display_t*         display_get();
    lv_display_rotation_t display_get_rotation();
    void                  display_set_rotation(lv_display_rotation_t rot);
    void                  display_set_auto_rotation_enabled(bool enabled);
    bool                  display_get_auto_rotation_enabled();
    void                  display_set_brightness(uint8_t pct);
    void                  display_sleep();
    void                  display_wakeup();
    void                  display_request_refresh();
    bool                  display_consume_refresh_request();
    void                  display_force_refresh();
}
