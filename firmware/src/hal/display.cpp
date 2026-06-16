// ============================================================
// CompagnonV2 — hal/display.cpp
// CO5300 AMOLED QSPI + LVGL display registration
// fix bandes noires : buf_size en octets = W * LINES * 2 (RGB565)
// Ecran physique 480x480, LVGL en plein ecran pour garder le meme
// repere que le touch. Les marges boitier restent disponibles
// uniquement comme safe area pour le layout.
//
// ROTATION : LV_DISPLAY_ROTATION_270 (-90°) corrige le rendu
// physique de la carte Waveshare ESP32-S3-Touch-AMOLED-2.16.
// LVGL transforme uniquement le rendu — les coords touch restent
// en rotation=0 (pass-through brut CST9220 deja aligne).
// Pour la rotation auto (QMI8658), appeler lv_display_set_rotation()
// via display_get() — c'est le seul point de changement necessaire.
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
static lv_display_t* _disp  = nullptr;

// Flush callback : repere 1:1 entre LVGL et la dalle physique
static void _flush_cb(lv_display_t* disp, const lv_area_t* area,
                      uint8_t* px_map) {
    co5300::flush(
        area->x1,
        area->y1,
        area->x2,
        area->y2,
        (const uint16_t*)px_map
    );
    lv_display_flush_ready(disp);
}

namespace hal {

void display_init() {
    // 1. Init hardware CO5300 en 480x480 physique (rotation=0 hardware)
    co5300::init();

    // 2. Alloue buffers (PSRAM preferee, sinon RAM interne)
    _buf1 = (uint8_t*)heap_caps_malloc(BUF_BYTES, MALLOC_CAP_SPIRAM);
    _buf2 = (uint8_t*)heap_caps_malloc(BUF_BYTES, MALLOC_CAP_SPIRAM);
    if (!_buf1 || !_buf2) {
        free(_buf1); free(_buf2);
        _buf1 = (uint8_t*)heap_caps_malloc(BUF_BYTES, MALLOC_CAP_INTERNAL);
        _buf2 = (uint8_t*)heap_caps_malloc(BUF_BYTES, MALLOC_CAP_INTERNAL);
    }
    if (!_buf1 || !_buf2) {
        Serial.println("[HAL] display_init: alloc FAILED");
        return;
    }

    // 3. Creation display LVGL sur le plein 480x480
    _disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    lv_display_set_flush_cb(_disp, _flush_cb);
    lv_display_set_buffers(_disp, _buf1, _buf2, BUF_BYTES,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    // ROTATION_270 (-90 deg) : corrige le rendu physique de la carte Waveshare.
    // LVGL tourne uniquement le rendu — les coords touch (rotation=0 pass-through)
    // ne sont PAS affectees car lv_display_set_rotation() avec LVGL owns rotation
    // ne remappe pas l'indev quand touch est deja aligne en brut.
    // Pour la rotation auto via QMI8658 : appeler lv_display_set_rotation()
    // avec display_get() depuis rotation_manager.
    lv_display_set_rotation(_disp, LV_DISPLAY_ROTATION_270);

    Serial.printf("[CO5300] init OK — rotation=270 (LVGL owns rotation)\n");
    Serial.printf("[HAL] display_init OK — phys=%dx%d lv=%dx%d buf=%u B x2\n",
                  LCD_WIDTH_PHYS, LCD_HEIGHT_PHYS,
                  LCD_WIDTH, LCD_HEIGHT, (unsigned)BUF_BYTES);
}

lv_display_t* display_get()  { return _disp; }

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
