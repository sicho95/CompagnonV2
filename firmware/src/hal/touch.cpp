// ============================================================
// CompagnonV2 — hal/touch.cpp
// CST9220 via SensorLib 0.4.1 — namespace hal
// IMPORTANT : PIN_TP_INT doit être configuré en INPUT avant
// l'init du driver, sinon isPressed() lit une valeur flottante
// et retourne toujours false.
// ============================================================
#include "touch.h"
#include "../../include/pins.h"
#include <Arduino.h>
#include <Wire.h>
#include <SensorLib.h>
#include <TouchDrv.hpp>

static TouchDrvCST92xx _drv;
static bool _ok = false;

static int32_t _clamp_i32(int32_t v, int32_t lo, int32_t hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void _map_touch_to_lvgl(int32_t raw_x, int32_t raw_y,
                               uint16_t& x, uint16_t& y) {
#if (LCD_ROTATION == 3)
    // Inverse du flush logiciel 90 degres anti-horaire :
    // phys_x = logical_y + LCD_MARGIN_V
    // phys_y = (LCD_WIDTH - 1 - logical_x) + LCD_MARGIN_H
    int32_t lx = (LCD_WIDTH - 1) - (raw_y - LCD_MARGIN_H);
    int32_t ly = raw_x - LCD_MARGIN_V;
#else
    int32_t lx = raw_x - LCD_MARGIN_H;
    int32_t ly = raw_y - LCD_MARGIN_V;
#endif
    x = (uint16_t)_clamp_i32(lx, 0, LCD_WIDTH  - 1);
    y = (uint16_t)_clamp_i32(ly, 0, LCD_HEIGHT - 1);
}

bool hal::touch_init() {
    // Configurer les GPIO touch avant l'init du driver
    pinMode(PIN_TP_INT, INPUT);   // CST9220 active-low interrupt
    pinMode(PIN_TP_RST, OUTPUT);
    digitalWrite(PIN_TP_RST, HIGH);
    delay(10);
    digitalWrite(PIN_TP_RST, LOW);
    delay(20);
    digitalWrite(PIN_TP_RST, HIGH);
    delay(100);  // attente stabilisation CST9220 post-reset

    // Wire déjà init par PMU — ne pas rappeler Wire.begin()
    if (_drv.begin(Wire, CST92XX_SLAVE_ADDRESS, PIN_TP_RST, PIN_TP_INT)) {
        _drv.setSwapXY(false);
        _drv.setMirrorXY(false, false);
        _ok = true;
        Serial.printf("[TOUCH] CST9220 OK — %s (RST=%d INT=%d)\n",
                      _drv.getModelName(), PIN_TP_RST, PIN_TP_INT);
    } else {
        _ok = false;
        Serial.printf("[TOUCH] CST9220 init FAILED (RST=%d INT=%d)\n",
                      PIN_TP_RST, PIN_TP_INT);
    }
    return _ok;
}

bool hal::touch_read(uint16_t& x, uint16_t& y) {
    if (!_ok) return false;
    if (_drv.isPressed()) {
        int16_t tx[1] = {0};
        int16_t ty[1] = {0};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        uint8_t n = _drv.getPoint(tx, ty, 1);
#pragma GCC diagnostic pop
        if (n > 0) {
            _map_touch_to_lvgl(tx[0], ty[0], x, y);
            return true;
        }
    }
    return false;
}

bool hal::touch_has_data() {
    return _ok && _drv.isPressed();
}
