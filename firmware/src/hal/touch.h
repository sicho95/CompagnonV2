// ============================================================
// CompagnonV2 — touch.h
// CST9220 I2C
// ============================================================
#pragma once
#include <stdint.h>

namespace hal {

bool touch_init();
void touch_read(lv_indev_t* indev, lv_indev_data_t* data);

} // namespace hal

// ── Alias flat C ───────────────────────────────────────────────
inline bool hal_touch_init() { return hal::touch_init(); }
