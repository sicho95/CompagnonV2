// ============================================================
// CompagnonV2 — hal/display.cpp
// CO5300 AMOLED QSPI + LVGL display registration
// ============================================================
#include "display.h"
#include "drivers/co5300.h"
#include "../../include/pins.h"
#include <lvgl.h>
#include <Arduino.h>

// Buffer LVGL en PSRAM (double buffer 1/10 écran)
#define BUF_LINES  (LCD_HEIGHT / 10)
static lv_color_t* _buf1 = nullptr;
static lv_color_t* _buf2 = nullptr;
static lv_display_t* _disp = nullptr;

// Flush callback LVGL → CO5300
static void _flush_cb(lv_display_t* disp, const lv_area_t* area,
                      uint8_t* px_map) {
    co5300::flush(area->x1, area->y1, area->x2, area->y2,
                  (const uint16_t*)px_map);
    lv_display_flush_ready(disp);
}

namespace hal {

void display_init() {
    // 1. Init hardware CO5300
    co5300::init();

    // 2. Alloc buffers en PSRAM
    size_t buf_size = LCD_WIDTH * BUF_LINES * sizeof(lv_color_t);
    _buf1 = (lv_color_t*)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    _buf2 = (lv_color_t*)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    if (!_buf1 || !_buf2) {
        Serial.println("[HAL] display_init: PSRAM alloc failed, trying internal RAM");
        _buf1 = (lv_color_t*)heap_caps_malloc(buf_size, MALLOC_CAP_INTERNAL);
        _buf2 = (lv_color_t*)heap_caps_malloc(buf_size, MALLOC_CAP_INTERNAL);
    }

    // 3. Création display LVGL
    _disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    lv_display_set_flush_cb(_disp, _flush_cb);
    lv_display_set_buffers(_disp, _buf1, _buf2, buf_size,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_rotation(_disp, LV_DISPLAY_ROTATION_0);

    Serial.printf("[HAL] display_init OK — %dx%d, buf=%u bytes x2\n",
                  LCD_WIDTH, LCD_HEIGHT, (unsigned)buf_size);
}

lv_display_t* display_get() {
    return _disp;
}

void display_set_brightness(uint8_t pct) {
    if (co5300::gfx()) co5300::gfx()->setBrightness(pct * 255 / 100);
}

void display_sleep() {
    co5300::sleep();
    Serial.println("[HAL] display_sleep");
}

void display_wakeup() {
    co5300::wakeup();
    Serial.println("[HAL] display_wakeup");
}

void display_force_refresh() {
    lv_refr_now(_disp);
}

} // namespace hal
