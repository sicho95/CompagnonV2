// ============================================================
// CompagnonV2 — display.cpp
// CO5300 AMOLED QSPI
// Pins: LCD_CS=GPIO12, QSPI_SCL=GPIO38, SIO0=GPIO4, SI1=GPIO5
//       SI2=GPIO6, SI3=GPIO7, RST=GPIO39
// ⚠️ GPIO38 partagé avec ES7210 MCLK — init display AVANT audio
// ============================================================
#include "display.h"
#include "../../../include/pins.h"
#include <Arduino.h>

// TODO: inclure le driver CO5300 QSPI (lib Waveshare ou Arduino_GFX)
// Placeholder — à remplacer par l'init QSPI réelle du CO5300

namespace hal {

bool display_init() {
    // Reset
    if (PIN_LCD_RST >= 0) {
        pinMode(PIN_LCD_RST, OUTPUT);
        digitalWrite(PIN_LCD_RST, LOW);
        delay(10);
        digitalWrite(PIN_LCD_RST, HIGH);
        delay(120);
    }
    // CS
    pinMode(PIN_LCD_CS, OUTPUT);
    digitalWrite(PIN_LCD_CS, HIGH);

    // TODO: init SPI QSPI (Arduino_GFX ou driver CO5300 dédié)
    // Arduino_GFX exemple :
    // Arduino_DataBus *bus = new Arduino_QSPI(PIN_LCD_CS, PIN_LCD_SCLK,
    //     PIN_LCD_SIO0, PIN_LCD_SI1, PIN_LCD_SI2, PIN_LCD_SI3);
    // gfx = new Arduino_CO5300(bus, PIN_LCD_RST, 0, false);
    // gfx->begin();

    Serial.println("[DISPLAY] CO5300 QSPI init placeholder");
    Serial.printf("  CS=%d SCL=%d SIO0=%d SI1=%d SI2=%d SI3=%d RST=%d\n",
        PIN_LCD_CS, PIN_LCD_SCLK, PIN_LCD_SIO0,
        PIN_LCD_SI1, PIN_LCD_SI2, PIN_LCD_SI3, PIN_LCD_RST);
    return true;
}

void display_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t* data) {
    // TODO: implémenter flush LVGL → CO5300 QSPI
}

void display_set_brightness(uint8_t pct) {
    // Pas de pin backlight sur AMOLED — gérer via AXP2101 si besoin
    (void)pct;
}

void display_sleep() {
    // TODO: commande sleep CO5300
}

void display_wakeup() {
    // TODO: commande wakeup CO5300
}

} // namespace hal
