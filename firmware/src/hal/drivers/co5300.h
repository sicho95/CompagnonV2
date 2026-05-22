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

static Arduino_DataBus* _bus = nullptr;
static Arduino_CO5300*  _gfx = nullptr;

void _cmd(uint8_t cmd) {
    if (_bus) _bus->writeC8D8(cmd, 0x00);
}

void init() {
    _bus = new Arduino_ESP32QSPI(
        PIN_LCD_CS, PIN_LCD_SCLK,
        PIN_LCD_SIO0, PIN_LCD_SI1, PIN_LCD_SI2, PIN_LCD_SI3
    );
    _gfx = new Arduino_CO5300(
        _bus, PIN_LCD_RST, 0,
        LCD_WIDTH, LCD_HEIGHT, 0, 0, 0, 0
    );

    if (!_gfx->begin()) {
        Serial.println("[CO5300] gfx->begin() FAILED");
        return;
    }

    _gfx->displayOn();        // MIPI 0x29 — obligatoire sinon ecran noir
    _gfx->setBrightness(200);
    _gfx->fillScreen(0x0000); // fond noir avant premier flush LVGL

    Serial.println("[CO5300] init OK");
}

void flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
           const uint16_t* color_map) {
    if (!_gfx) return;
    _gfx->draw16bitRGBBitmap(x1, y1, (uint16_t*)color_map, x2-x1+1, y2-y1+1);
}

Arduino_CO5300* gfx() { return _gfx; }

} // namespace co5300
