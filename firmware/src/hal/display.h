// ============================================================
// CompagnonV2 — display.h
// CO5300 QSPI AMOLED
// LCD_CS=GPIO12, QSPI_SCL=GPIO38, SIO0-3=GPIO4-7, RST=GPIO39
// ============================================================
#pragma once
#include <stdint.h>

namespace hal {

bool   display_init();
void   display_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t* data);
void   display_set_brightness(uint8_t pct); // 0-100 via AXP ou LEDC si dispo
void   display_sleep();
void   display_wakeup();

} // namespace hal
