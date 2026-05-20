// ============================================================
// CompagnonV2 — display.cpp
// CO5300 AMOLED QSPI
// lv_init() est appelé dans le .ino avant hal_display_init()
// Ce fichier ne fait QUE lv_display_create() + buffers
// ============================================================
#include "display.h"
#include "../../include/pins.h"
#include <Arduino.h>

namespace hal {

static lv_display_t* _disp = nullptr;

// Double draw buffer statique (10 lignes x 536px x 2 octets/px = ~10KB chacun)
#define DISP_W    536
#define DISP_H    240
#define BUF_LINES  10
static lv_color_t _buf1[DISP_W * BUF_LINES];
static lv_color_t _buf2[DISP_W * BUF_LINES];

// Flush no-op : sera remplacée par le vrai driver CO5300 QSPI
static void _flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    (void)area; (void)px_map;
    lv_display_flush_ready(disp);
}

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

    // lv_init() déjà appelé dans setup() — on crée directement le display
    _disp = lv_display_create(DISP_W, DISP_H);
    lv_display_set_flush_cb(_disp, _flush_cb);
    lv_display_set_buffers(_disp, _buf1, _buf2,
                           sizeof(_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    Serial.printf("[DISPLAY] CO5300 QSPI — LVGL display stub OK (536x240)\n"
                  "  CS=%d SCL=%d SIO0=%d SI1=%d SI2=%d SI3=%d RST=%d\n",
                  PIN_LCD_CS, PIN_LCD_SCLK, PIN_LCD_SIO0,
                  PIN_LCD_SI1, PIN_LCD_SI2, PIN_LCD_SI3, PIN_LCD_RST);
    return true;
}

lv_display_t* display_get() {
    return _disp;
}

void display_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t* data) {
    (void)x1; (void)y1; (void)x2; (void)y2; (void)data;
}

void display_set_brightness(uint8_t pct) { (void)pct; }
void display_sleep()   { /* TODO */ }
void display_wakeup()  { /* TODO */ }

} // namespace hal
