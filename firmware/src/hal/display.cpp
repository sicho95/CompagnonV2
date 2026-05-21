// ============================================================
// CompagnonV2 — display.cpp
// CO5300 AMOLED QSPI — driver réel
// ============================================================
#include "display.h"
#include "drivers/co5300.h"
#include "../../include/pins.h"
#include <Arduino.h>

namespace hal {

static lv_display_t* _disp = nullptr;

#define DISP_W    536
#define DISP_H    240
#define BUF_LINES  20

// Buffers en PSRAM si dispo, sinon IRAM
static lv_color_t _buf1[DISP_W * BUF_LINES];
static lv_color_t _buf2[DISP_W * BUF_LINES];

// Callback LVGL → CO5300
static void _flush_cb(lv_display_t* disp,
                      const lv_area_t* area,
                      uint8_t* px_map) {
    co5300::flush(area->x1, area->y1, area->x2, area->y2,
                  (const uint16_t*)px_map);
    lv_display_flush_ready(disp);
}

bool display_init() {
    co5300::init();

    _disp = lv_display_create(DISP_W, DISP_H);
    lv_display_set_flush_cb(_disp, _flush_cb);
    lv_display_set_buffers(_disp, _buf1, _buf2,
                           sizeof(_buf1),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    Serial.printf("[DISPLAY] CO5300 QSPI — driver OK (536x240)\n"
                  "  CS=%d SCK=%d D0=%d D1=%d D2=%d D3=%d RST=%d\n",
                  PIN_LCD_CS, PIN_LCD_SCLK,
                  PIN_LCD_SIO0, PIN_LCD_SI1,
                  PIN_LCD_SI2, PIN_LCD_SI3,
                  PIN_LCD_RST);
    return true;
}

lv_display_t* display_get()  { return _disp; }

void display_flush(int32_t x1, int32_t y1,
                   int32_t x2, int32_t y2,
                   const uint16_t* data) {
    co5300::flush(x1, y1, x2, y2, data);
}

void display_set_brightness(uint8_t pct) { (void)pct; /* géré par PMU */ }
void display_sleep()   { co5300::_cmd(0x10); } // SLPIN
void display_wakeup()  { co5300::_cmd(0x11); delay(120); } // SLPOUT

} // namespace hal
