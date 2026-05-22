// ============================================================
// CompagnonV2 — hal/display.cpp
// CO5300 AMOLED QSPI — 466x466
// Rotation panel : 270° (valide par test bandes fillRect)
// ============================================================
#include "display.h"
#include "drivers/co5300.h"
#include "../../include/pins.h"
#include <Arduino.h>

namespace hal {

static lv_display_t* _disp = nullptr;

#define DISP_W    LCD_WIDTH
#define DISP_H    LCD_HEIGHT
#define BUF_PIXELS ((size_t)DISP_W * DISP_H)

static lv_color_t* _buf1 = nullptr;

static void _flush_cb(lv_display_t* disp,
                      const lv_area_t* area,
                      uint8_t* px_map) {
    co5300::flush(area->x1, area->y1, area->x2, area->y2,
                  (const uint16_t*)px_map);
    lv_display_flush_ready(disp);
}

bool display_init() {
    co5300::init();

    size_t buf_sz = BUF_PIXELS * sizeof(lv_color_t);
    _buf1 = (lv_color_t*)ps_malloc(buf_sz);
    if (!_buf1) {
        Serial.println("[DISPLAY] ERREUR: ps_malloc FULL buffer impossible");
        buf_sz = (size_t)DISP_W * 40 * sizeof(lv_color_t);
        _buf1 = (lv_color_t*)ps_malloc(buf_sz);
        if (!_buf1) _buf1 = (lv_color_t*)malloc(buf_sz);
        _disp = lv_display_create(DISP_W, DISP_H);
        lv_display_set_flush_cb(_disp, _flush_cb);
        lv_display_set_buffers(_disp, _buf1, nullptr, buf_sz,
                               LV_DISPLAY_RENDER_MODE_PARTIAL);
        Serial.println("[DISPLAY] WARN: fallback PARTIAL 40 lignes");
    } else {
        _disp = lv_display_create(DISP_W, DISP_H);
        lv_display_set_flush_cb(_disp, _flush_cb);
        lv_display_set_buffers(_disp, _buf1, nullptr, buf_sz,
                               LV_DISPLAY_RENDER_MODE_FULL);
        Serial.println("[DISPLAY] RENDER_MODE_FULL actif (PSRAM)");
    }

    // Rotation 270° : valide par test bandes (panel physique tourne de 90° en trop sans ca)
    lv_display_set_rotation(_disp, LV_DISPLAY_ROTATION_270);

    Serial.printf("[DISPLAY] CO5300 QSPI — driver OK (%dx%d)\n"
                  "  CS=%d SCK=%d D0=%d D1=%d D2=%d D3=%d RST=%d\n"
                  "  buf_sz=%u bytes\n",
                  DISP_W, DISP_H,
                  PIN_LCD_CS, PIN_LCD_SCLK,
                  PIN_LCD_SIO0, PIN_LCD_SI1,
                  PIN_LCD_SI2, PIN_LCD_SI3,
                  PIN_LCD_RST,
                  (unsigned)buf_sz);
    return (_disp != nullptr);
}

lv_display_t* display_get() { return _disp; }

void display_flush(int32_t x1, int32_t y1,
                   int32_t x2, int32_t y2,
                   const uint16_t* data) {
    co5300::flush(x1, y1, x2, y2, data);
}

void display_set_brightness(uint8_t pct) {
    Arduino_CO5300* g = co5300::gfx();
    if (g) g->setBrightness((uint8_t)((uint32_t)pct * 255 / 100));
}

void display_force_refresh() {
    if (_disp) {
        lv_obj_invalidate(lv_scr_act());
        lv_refr_now(_disp);
    }
}

void display_sleep()  { co5300::_cmd(0x10); }
void display_wakeup() { co5300::_cmd(0x11); delay(120); }

} // namespace hal
