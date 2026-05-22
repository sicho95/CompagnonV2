// ============================================================
// CompagnonV2 — drivers/co5300.cpp
// CO5300 AMOLED QSPI — implémentation
// ============================================================
#include "co5300.h"

namespace co5300 {

static Arduino_DataBus* _bus = nullptr;
static Arduino_CO5300*  _gfx = nullptr;

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

    _gfx->displayOn();        // MIPI 0x29 — obligatoire sinon écran noir
    _gfx->setBrightness(200);
    _gfx->fillScreen(0x0000); // fond noir avant premier flush LVGL

    Serial.println("[CO5300] init OK");
}

void flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
           const uint16_t* color_map) {
    if (!_gfx) return;
    _gfx->draw16bitRGBBitmap(x1, y1, (uint16_t*)color_map,
                              x2 - x1 + 1, y2 - y1 + 1);
}

Arduino_CO5300* gfx() { return _gfx; }

// SLPIN / SLPOUT : writeC8D8 est incorrect pour des commandes MIPI DCS
// sans paramètre — utiliser l'API haut niveau de Arduino_GFX
void sleep() {
    if (!_gfx) return;
    _gfx->displayOff(); // MIPI 0x28
    delay(5);
}

void wakeup() {
    if (!_gfx) return;
    _gfx->displayOn();  // MIPI 0x29
    delay(120);         // recovery time CO5300 datasheet
}

} // namespace co5300
