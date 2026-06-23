// ============================================================
// CompagnonV2 — touch.h
// CST9220 I2C
// fix: suppression lv_indev_t / lv_indev_data_t (ancienne signature LVGL)
//      signature réelle : bool touch_read(uint16_t& x, uint16_t& y)
// ============================================================
#pragma once
#include <stdint.h>

namespace hal {

constexpr uint8_t TOUCH_MAX_POINTS = 5;

struct TouchPoint {
    bool     valid;
    int16_t  raw_x;
    int16_t  raw_y;
    uint16_t x;
    uint16_t y;
};

struct TouchFrame {
    bool     pressed;
    bool     just_pressed;
    bool     just_released;
    uint8_t  point_count;
    uint32_t timestamp_ms;
    TouchPoint points[TOUCH_MAX_POINTS];
};

bool touch_init();
bool touch_update();
bool touch_read(uint16_t& x, uint16_t& y);
bool touch_has_data();
const TouchFrame& touch_frame();

} // namespace hal

// ── Alias flat C ───────────────────────────────────────────────
inline bool hal_touch_init() { return hal::touch_init(); }
