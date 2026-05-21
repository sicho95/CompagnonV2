#pragma once
// ============================================================
// CompagnonV2 — drivers/co5300.h
// CO5300 AMOLED QSPI — Waveshare ESP32-S3-Touch-AMOLED-2.16"
// Bus : QSPI  CS=12 SCK=38 D0=4 D1=5 D2=6 D3=7  RST=2
// Résolution : 480 x 480 (carré)
// Protocole  : Arduino_GFX Arduino_ESP32QSPI + Arduino_CO5300
// Référence  : code officiel Waveshare (examples/Arduino-v3.3.5)
// ============================================================
#include <Arduino.h>
#include "Arduino_GFX_Library.h"
#include "../../../include/pins.h"

namespace co5300 {

static Arduino_DataBus* _bus = nullptr;
static Arduino_CO5300*  _gfx = nullptr;

void _cmd(uint8_t cmd) {
    if (_bus) _bus->writeC8D8(cmd, 0x00);
}

void init() {
    _bus = new Arduino_ESP32QSPI(
        PIN_LCD_CS,
        PIN_LCD_SCLK,
        PIN_LCD_SIO0,
        PIN_LCD_SI1,
        PIN_LCD_SI2,
        PIN_LCD_SI3
    );
    _gfx = new Arduino_CO5300(
        _bus,
        PIN_LCD_RST,
        0 /* rotation */,
        LCD_WIDTH,
        LCD_HEIGHT,
        0, 0, 0, 0
    );

    if (!_gfx->begin()) {
        Serial.println("[CO5300] gfx->begin() FAILED");
        return;
    }
    // Orientation register — obligatoire post-init (Waveshare)
    _bus->writeC8D8(0x36, 0xA0);

    _gfx->fillScreen(BLACK);
    _gfx->setBrightness(200);
}

void flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
           const uint16_t* color_map) {
    if (!_gfx) return;
    uint32_t w = x2 - x1 + 1;
    uint32_t h = y2 - y1 + 1;
    _gfx->draw16bitRGBBitmap(x1, y1, (uint16_t*)color_map, w, h);
}

Arduino_GFX* gfx() { return _gfx; }

} // namespace co5300
