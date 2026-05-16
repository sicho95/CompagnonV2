// ============================================================
// CompagnonV2 — display.cpp
// CO5300 AMOLED QSPI
// ============================================================
#include "display.h"
#include "../../include/pins.h"
#include <Arduino.h>

namespace hal {

static lv_display_t* _disp = nullptr;

bool display_init() {
    if (PIN_LCD_RST >= 0) {
        pinMode(PIN_LCD_RST, OUTPUT);
        digitalWrite(PIN_LCD_RST, LOW);
        delay(10);
        digitalWrite(PIN_LCD_RST, HIGH);
        delay(120);
    }
    pinMode(PIN_LCD_CS, OUTPUT);
    digitalWrite(PIN_LCD_CS, HIGH);

    // TODO: init SPI QSPI réelle (Arduino_GFX / driver CO5300)
    // _disp = lv_display_create(536, 240);
    // lv_display_set_flush_cb(_disp, display_flush_cb);

    Serial.printf("[DISPLAY] CO5300 QSPI init placeholder\n"
                  "  CS=%d SCL=%d SIO0=%d SI1=%d SI2=%d SI3=%d RST=%d\n",
                  PIN_LCD_CS, PIN_LCD_SCLK, PIN_LCD_SIO0,
                  PIN_LCD_SI1, PIN_LCD_SI2, PIN_LCD_SI3, PIN_LCD_RST);
    return true;
}

lv_display_t* display_get() {
    return _disp;
}

void display_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t* data) {
    // TODO: implémenter flush LVGL → CO5300 QSPI
    (void)x1; (void)y1; (void)x2; (void)y2; (void)data;
}

void display_set_brightness(uint8_t pct) {
    (void)pct; // AMOLED : pas de backlight, gérer via AXP2101 si nécessaire
}

void display_sleep()   { /* TODO: commande sleep CO5300   */ }
void display_wakeup()  { /* TODO: commande wakeup CO5300  */ }

} // namespace hal
