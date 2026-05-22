#pragma once
// ============================================================
// CompagnonV2 — drivers/co5300.h  [DIAG ENDIANNESS]
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

    _gfx->displayOn();
    _gfx->setBrightness(200);
    _gfx->fillScreen(0x0000); // fond noir
    delay(500);

    // ── DIAG BANDES : 4 rectangles de 466x116 pixels chacun
    // Tester les deux endianness pour chaque couleur
    // Si tu vois des couleurs => on sait quelle endianness marche
    // Si tout est noir => displayOn/fillScreen ne fonctionne pas du tout

    // Bande 1 (haut) : ROUGE Little Endian  0xF800
    _gfx->fillScreen(0x0000);
    _gfx->fillRect(0,   0, 466, 116, 0xF800); // Rouge LE
    _gfx->fillRect(0, 116, 466, 116, 0x07E0); // Vert  LE
    _gfx->fillRect(0, 232, 466, 116, 0x001F); // Bleu  LE
    _gfx->fillRect(0, 348, 466, 118, 0xFFFF); // Blanc LE
    Serial.println("[CO5300] DIAG: bandes Rouge/Vert/Bleu/Blanc (LE) affichees");
    Serial.println("[CO5300] Si ecran noir => displayOn/fillRect KO");
    Serial.println("[CO5300] Si couleurs OK => endianness LE correct, swap LVGL inutile");
    Serial.println("[CO5300] Si couleurs inversees => swap necessaire");

    Serial.println("[CO5300] init OK");
}

void flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
           const uint16_t* color_map) {
    if (!_gfx) return;
    _gfx->draw16bitRGBBitmap(x1, y1, (uint16_t*)color_map, x2-x1+1, y2-y1+1);
}

Arduino_CO5300* gfx() { return _gfx; }

} // namespace co5300
