#pragma once
// ============================================================
// CompagnonV2 — drivers/co5300.h
// CO5300 AMOLED QSPI — Waveshare ESP32-S3-Touch-AMOLED-2.16"
// Bus : QSPI  CS=12 SCK=38 D0=4 D1=5 D2=6 D3=7  RST=2
// Résolution : 466 x 466
// ============================================================
#include <Arduino.h>
#include "Arduino_GFX_Library.h"
#include "../../../include/pins.h"

namespace co5300 {

void init();
void flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
           const uint16_t* color_map);

Arduino_CO5300* gfx();
void sleep();
void wakeup();

} // namespace co5300
