#pragma once
#include <lvgl.h>

namespace hal {
    void          display_init();
    lv_display_t* display_get();
    void          display_set_brightness(uint8_t pct);
    void          display_sleep();
    void          display_wakeup();
    void          display_force_refresh();
}
