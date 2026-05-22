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

// Aliases style C-flat pour compatibilite avec CompagnonV2.ino
// (meme convention que hal_audio_init, hal_touch_init, etc.)
inline void          hal_display_init()                      { hal::display_init(); }
inline lv_display_t* hal_display_get()                       { return hal::display_get(); }
inline void          hal_display_set_brightness(uint8_t pct) { hal::display_set_brightness(pct); }
inline void          hal_display_sleep()                     { hal::display_sleep(); }
inline void          hal_display_wakeup()                    { hal::display_wakeup(); }
inline void          hal_display_force_refresh()             { hal::display_force_refresh(); }
