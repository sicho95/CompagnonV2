// ============================================================
// CompagnonV2 — drivers/co5300.cpp
// CO5300 AMOLED QSPI — implémentation
// IMPORTANT : init en 480x480 PHYSIQUE.
// L'offset boîtier (LCD_MARGIN_H/V) est appliqué dans display.cpp
// lors du flush. Si le GFX est init en 440x460, draw16bitRGBBitmap
// clampe les coords et l'image reste collée en haut-gauche.
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
    // Init en résolution physique complète 480x480
    // (pas en zone utile 440x460)
    _gfx = new Arduino_CO5300(
        _bus, PIN_LCD_RST, 0,
        LCD_WIDTH_PHYS, LCD_HEIGHT_PHYS, 0, 0, 0, 0
    );

    if (!_gfx->begin()) {
        Serial.println("[CO5300] gfx->begin() FAILED");
        return;
    }
    _gfx->setRotation(LCD_ROTATION);
    _gfx->displayOn();
    _gfx->setBrightness(200);
    _gfx->fillScreen(0x0000);
    Serial.printf("[CO5300] init OK — rotation=%d\n", LCD_ROTATION);
}

void flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
           const uint16_t* color_map) {
    if (!_gfx) return;
    _gfx->draw16bitRGBBitmap(x1, y1, (uint16_t*)color_map,
                              x2 - x1 + 1, y2 - y1 + 1);
}

Arduino_CO5300* gfx() { return _gfx; }

void sleep() {
    if (!_gfx) return;
    _gfx->displayOff();
    delay(5);
}

void wakeup() {
    if (!_gfx) return;
    _gfx->displayOn();
    delay(120);
}

} // namespace co5300
