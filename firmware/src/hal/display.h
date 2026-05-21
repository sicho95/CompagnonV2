#pragma once
#include <lvgl.h>
#include <stdint.h>

namespace hal {
    bool           display_init();
    lv_display_t*  display_get();
    void           display_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t* data);
    void           display_set_brightness(uint8_t pct); // 0-100
    void           display_force_refresh();             // force lv_refr_now()
    void           display_sleep();
    void           display_wakeup();
}

// Alias C pour le .ino
inline bool           hal_display_init()           { return hal::display_init(); }
inline lv_display_t*  hal_display_get()            { return hal::display_get(); }
inline void           hal_display_set_brightness(uint8_t p) { hal::display_set_brightness(p); }
inline void           hal_display_force_refresh()  { hal::display_force_refresh(); }
