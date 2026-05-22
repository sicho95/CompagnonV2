#include "display.h"
#include "drivers/co5300.h"
#include "../../include/pins.h"
#include <Arduino.h>
#include <lvgl.h>

#define DISP_W  LCD_WIDTH
#define DISP_H  LCD_HEIGHT
#define BUF_LINES 40

static lv_display_t* s_disp = nullptr;
static lv_color_t*   s_buf1 = nullptr;
static lv_color_t*   s_buf2 = nullptr;

static void flush_cb(lv_display_t* disp,
                     const lv_area_t* area,
                     uint8_t* px_map) {
    co5300::flush(area->x1, area->y1, area->x2, area->y2,
                  (const uint16_t*)px_map);
    lv_display_flush_ready(disp);
}

void hal_display_init() {
    co5300::init();

    size_t buf_sz = (size_t)DISP_W * BUF_LINES * sizeof(lv_color_t);
    s_buf1 = (lv_color_t*)ps_malloc(buf_sz);
    s_buf2 = (lv_color_t*)ps_malloc(buf_sz);
    if (!s_buf1) s_buf1 = (lv_color_t*)malloc(buf_sz);
    if (!s_buf2) s_buf2 = (lv_color_t*)malloc(buf_sz);

    s_disp = lv_display_create(DISP_W, DISP_H);
    lv_display_set_flush_cb(s_disp, flush_cb);
    lv_display_set_buffers(s_disp, s_buf1, s_buf2, buf_sz,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_rotation(s_disp, LV_DISPLAY_ROTATION_270);

    Serial.printf("[DISPLAY] CO5300 QSPI — driver OK (%dx%d)\n"
                  "  CS=%d SCK=%d D0=%d D1=%d D2=%d D3=%d RST=%d\n"
                  "  buf_sz=%u bytes\n",
                  DISP_W, DISP_H,
                  PIN_LCD_CS, PIN_LCD_SCLK,
                  PIN_LCD_SIO0, PIN_LCD_SI1,
                  PIN_LCD_SI2, PIN_LCD_SI3,
                  PIN_LCD_RST,
                  (unsigned)buf_sz);
}

lv_display_t* hal_display_get()  { return s_disp; }

void hal_display_set_brightness(uint8_t pct) {
    Arduino_CO5300* g = co5300::gfx();
    if (g) g->setBrightness((uint8_t)((uint32_t)pct * 255 / 100));
}

void hal_display_sleep()  { co5300::_cmd(0x10); }
void hal_display_wakeup() { co5300::_cmd(0x11); delay(120); }
