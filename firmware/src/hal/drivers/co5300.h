#pragma once
// ============================================================
// CompagnonV2 — drivers/co5300.h
// CO5300 AMOLED QSPI — Waveshare ESP32-S3-Touch-AMOLED-2.16"
// Bus : QSPI  CS=12 SCK=38 D0=4 D1=5 D2=6 D3=7  RST=2
// Résolution : 466 x 466 (source officielle Waveshare pin_config.h)
// Luminosité : gfx->setBrightness(0-255) — PAS de PIN_LCD_BL sur ce board
// Orientation : PAS de writeC8D8(0x36) ici — géré par LVGL via lv_display_set_rotation()
//               appelé automatiquement dans loop() selon hal_imu_orientation()
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
        0 /* rotation — laissé à 0, LVGL gère via lv_display_set_rotation() */,
        LCD_WIDTH,
        LCD_HEIGHT,
        0, 0, 0, 0
    );

    if (!_gfx->begin()) {
        Serial.println("[CO5300] gfx->begin() FAILED");
        return;
    }

    // NE PAS envoyer writeC8D8(0x36, 0xA0) ici :
    // ce registre MADCTL swap les axes hardware et rend le flush LVGL invisible.
    // L'orientation est gérée logiquement par LVGL via lv_display_set_rotation()
    // déclenchée dans loop() par hal_imu_changed() / hal_imu_orientation().

    // Effacer l'écran
    _gfx->fillScreen(RGB565_BLACK);

    // Luminosité via setBrightness() — ce board n'a PAS de broche PIN_LCD_BL séparée
    // 200/255 ≈ 78% (valeur recommandée Waveshare)
    _gfx->setBrightness(200);

    Serial.println("[CO5300] init OK");
}

void flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
           const uint16_t* color_map) {
    if (!_gfx) return;
    uint32_t w = x2 - x1 + 1;
    uint32_t h = y2 - y1 + 1;
    _gfx->draw16bitRGBBitmap(x1, y1, (uint16_t*)color_map, w, h);
}

// Retourne le type concret Arduino_CO5300* pour accéder à setBrightness()
Arduino_CO5300* gfx() { return _gfx; }

} // namespace co5300
