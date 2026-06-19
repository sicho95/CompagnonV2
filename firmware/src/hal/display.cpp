// ============================================================
// CompagnonV2 — hal/display.cpp
// CO5300 AMOLED QSPI + LVGL display registration
// fix bandes noires : buf_size en octets = W * LINES * 2 (RGB565)
// Ecran physique 480x480, LVGL en plein ecran pour garder le meme
// repere que le touch. Les marges boitier restent disponibles
// uniquement comme safe area pour le layout.
//
// ROTATION :
// - le CO5300 ne sait pas faire une vraie rotation 90/270
// - LCD_ROTATION est donc appliquée ici, dans le flush logiciel
// - touch.cpp mappe les points CST9220 dans le meme repere visible
// ============================================================
#include "display.h"
#include "drivers/co5300.h"
#include "../../include/pins.h"
#include <lvgl.h>
#include <Arduino.h>

// 10 lignes logiques par buffer
#define BUF_LINES  10

// Taille en octets — RGB565 = 2 octets/pixel
#define BUF_BYTES  (LCD_WIDTH * BUF_LINES * 2)

static uint8_t*      _buf1  = nullptr;
static uint8_t*      _buf2  = nullptr;
static uint16_t*     _rot_buf = nullptr;
static lv_display_t* _disp  = nullptr;
static lv_display_rotation_t _ui_rot = LV_DISPLAY_ROTATION_0;
static bool _auto_rotation_enabled = true;
static volatile bool _refresh_requested = false;

static lv_display_rotation_t _compose_rotation(uint8_t a, uint8_t b) {
    return (lv_display_rotation_t)((a + b) & 0x3);
}

static lv_display_rotation_t _panel_mount_rotation() {
    // LCD_ROTATION vient de l'ancien réglage Arduino_GFX/MADCTL.
    // En rotation logicielle, le sens est inverse.
    return (lv_display_rotation_t)((4 - (LCD_ROTATION & 0x3)) & 0x3);
}

static void _flush_rotated(const lv_area_t* area, const uint16_t* src,
                           lv_display_rotation_t rot) {
    const int32_t src_w = area->x2 - area->x1 + 1;
    const int32_t src_h = area->y2 - area->y1 + 1;
    int32_t dst_x1 = area->x1;
    int32_t dst_y1 = area->y1;
    int32_t dst_w = src_w;
    int32_t dst_h = src_h;

    if (rot == LV_DISPLAY_ROTATION_0) {
        co5300::flush(dst_x1, dst_y1, area->x2, area->y2, src);
        return;
    }

    if (!_rot_buf) return;

    switch (rot) {
        case LV_DISPLAY_ROTATION_90:
            dst_x1 = area->y1;
            dst_y1 = LCD_WIDTH - 1 - area->x2;
            dst_w = src_h;
            dst_h = src_w;
            for (int32_t sy = 0; sy < src_h; ++sy) {
                for (int32_t sx = 0; sx < src_w; ++sx) {
                    const int32_t dx = sy;
                    const int32_t dy = src_w - 1 - sx;
                    _rot_buf[dy * dst_w + dx] = src[sy * src_w + sx];
                }
            }
            break;
        case LV_DISPLAY_ROTATION_180:
            dst_x1 = LCD_WIDTH - 1 - area->x2;
            dst_y1 = LCD_HEIGHT - 1 - area->y2;
            dst_w = src_w;
            dst_h = src_h;
            for (int32_t sy = 0; sy < src_h; ++sy) {
                for (int32_t sx = 0; sx < src_w; ++sx) {
                    const int32_t dx = src_w - 1 - sx;
                    const int32_t dy = src_h - 1 - sy;
                    _rot_buf[dy * dst_w + dx] = src[sy * src_w + sx];
                }
            }
            break;
        case LV_DISPLAY_ROTATION_270:
            dst_x1 = LCD_HEIGHT - 1 - area->y2;
            dst_y1 = area->x1;
            dst_w = src_h;
            dst_h = src_w;
            for (int32_t sy = 0; sy < src_h; ++sy) {
                for (int32_t sx = 0; sx < src_w; ++sx) {
                    const int32_t dx = src_h - 1 - sy;
                    const int32_t dy = sx;
                    _rot_buf[dy * dst_w + dx] = src[sy * src_w + sx];
                }
            }
            break;
        default:
            return;
    }

    co5300::flush(dst_x1, dst_y1, dst_x1 + dst_w - 1, dst_y1 + dst_h - 1,
                  _rot_buf);
}

// Flush callback : rotation logicielle du buffer LVGL vers la dalle physique.
static void _flush_cb(lv_display_t* disp, const lv_area_t* area,
                      uint8_t* px_map) {
    (void)disp;
    _flush_rotated(area, (const uint16_t*)px_map, hal::display_get_rotation());
    lv_display_flush_ready(disp);
}

namespace hal {

void display_init() {
    // 1. Init hardware CO5300 en 480x480 physique, sans rotation matérielle.
    co5300::init();

    // 2. Alloue buffers (PSRAM preferee, sinon RAM interne)
    _buf1 = (uint8_t*)heap_caps_malloc(BUF_BYTES, MALLOC_CAP_SPIRAM);
    _buf2 = (uint8_t*)heap_caps_malloc(BUF_BYTES, MALLOC_CAP_SPIRAM);
    if (!_buf1 || !_buf2) {
        free(_buf1); free(_buf2);
        _buf1 = (uint8_t*)heap_caps_malloc(BUF_BYTES, MALLOC_CAP_INTERNAL);
        _buf2 = (uint8_t*)heap_caps_malloc(BUF_BYTES, MALLOC_CAP_INTERNAL);
    }
    _rot_buf = (uint16_t*)heap_caps_malloc(BUF_BYTES, MALLOC_CAP_SPIRAM);
    if (!_rot_buf) {
        _rot_buf = (uint16_t*)heap_caps_malloc(BUF_BYTES, MALLOC_CAP_INTERNAL);
    }
    if (!_buf1 || !_buf2 || !_rot_buf) {
        Serial.println("[HAL] display_init: alloc FAILED");
        return;
    }

    // 3. Creation display LVGL sur le plein 480x480
    _disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    lv_display_set_flush_cb(_disp, _flush_cb);
    lv_display_set_buffers(_disp, _buf1, _buf2, BUF_BYTES,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    // LVGL reste en rotation 0 : la rotation est faite dans _flush_cb.
    lv_display_set_rotation(_disp, LV_DISPLAY_ROTATION_0);

    Serial.printf("[HAL] LVGL init OK — mount_rotation=%d ui_rotation=%d flush_rotation=%d\n",
                  (int)_panel_mount_rotation(), (int)_ui_rot,
                  (int)display_get_rotation());
    Serial.printf("[HAL] display_init OK — phys=%dx%d lv=%dx%d buf=%u B x2\n",
                  LCD_WIDTH_PHYS, LCD_HEIGHT_PHYS,
                  LCD_WIDTH, LCD_HEIGHT, (unsigned)BUF_BYTES);
}

lv_display_t* display_get() { return _disp; }

lv_display_rotation_t display_get_rotation() {
    return _compose_rotation((uint8_t)_panel_mount_rotation(), (uint8_t)_ui_rot);
}

void display_set_rotation(lv_display_rotation_t rot) {
    if (!_disp) return;
    if (_ui_rot == rot) return;
    _ui_rot = rot;
    lv_display_set_rotation(_disp, LV_DISPLAY_ROTATION_0);
    // Rotation de flush: il faut redessiner toute la scène dans le nouveau repère.
    lv_obj_invalidate(lv_screen_active());
    lv_obj_invalidate(lv_layer_top());
    lv_refr_now(_disp);
    Serial.printf("[HAL] display_set_rotation -> ui=%d flush=%d\n",
                  (int)_ui_rot, (int)display_get_rotation());
}

void display_set_auto_rotation_enabled(bool enabled) {
    _auto_rotation_enabled = enabled;
    Serial.printf("[HAL] auto_rotation=%d\n", enabled ? 1 : 0);
}

bool display_get_auto_rotation_enabled() {
    return _auto_rotation_enabled;
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

void display_request_refresh() {
    _refresh_requested = true;
}

bool display_consume_refresh_request() {
    if (!_refresh_requested) return false;
    _refresh_requested = false;
    return true;
}

void display_force_refresh() {
    lv_refr_now(_disp);
}

} // namespace hal
