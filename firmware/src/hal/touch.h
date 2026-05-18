// ============================================================
// CompagnonV2 — touch.h
// CST9220 I2C
// fix: suppression lv_indev_t / lv_indev_data_t (ancienne signature LVGL)
//      signature réelle : bool touch_read(uint16_t& x, uint16_t& y)
// ============================================================
#pragma once
#include <stdint.h>

namespace hal {

bool touch_init();
bool touch_read(uint16_t& x, uint16_t& y);
bool touch_has_data();

} // namespace hal

// ── Alias flat C ───────────────────────────────────────────────
inline bool hal_touch_init() { return hal::touch_init(); }
