// ============================================================
// CompagnonV2 — display.h
// CO5300 QSPI AMOLED
// ============================================================
#pragma once
#include <stdint.h>
#include <lvgl.h>

namespace hal {

bool         display_init();
lv_display_t* display_get();   // retourne le handle LVGL (utilisé pour set_rotation)
void         display_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t* data);
void         display_set_brightness(uint8_t pct);
void         display_sleep();
void         display_wakeup();

} // namespace hal

// ── Aliases flat C pour le .ino ──────────────────────────────────
inline bool          hal_display_init() { return hal::display_init(); }
inline lv_display_t* hal_display_get()  { return hal::display_get(); }
