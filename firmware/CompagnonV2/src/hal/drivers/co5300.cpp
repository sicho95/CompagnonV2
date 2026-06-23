// ============================================================
// CompagnonV2 — drivers/co5300.cpp
// CO5300 AMOLED QSPI — implémentation
// IMPORTANT : init en 480x480 PHYSIQUE.
// Les marges boitier sont une safe area UI uniquement. Ne pas réduire
// le driver ou le display LVGL, sinon le tactile 480x480 se désaligne.
//
// ROTATION : le CO5300 ne sait pas faire une vraie rotation 90/270.
// Le driver reste donc en rotation matérielle 0 ; la rotation d'image
// est faite dans le flush LVGL.
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
    // Init en résolution physique complète 480x480.
    _gfx = new Arduino_CO5300(
        _bus, PIN_LCD_RST, 0,
        LCD_WIDTH_PHYS, LCD_HEIGHT_PHYS, 0, 0, 0, 0
    );

    if (!_gfx->begin()) {
        Serial.println("[CO5300] gfx->begin() FAILED");
        return;
    }
    // Le CO5300 ne supporte que des flips MADCTL, pas une vraie rotation 90/270.
    _gfx->setRotation(0);
    _gfx->displayOn();
    _gfx->setBrightness(200);
    _gfx->fillScreen(0x0000);
    Serial.printf("[CO5300] init OK — hw_rotation=0 (software rotation=%d)\n",
                  LCD_ROTATION);
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
