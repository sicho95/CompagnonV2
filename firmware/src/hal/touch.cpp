// ============================================================
// CompagnonV2 — hal/touch.cpp
// CST9220 via SensorLib — namespace hal
// Les coordonnées brutes CST9220 sont en 480x480 physique.
// On les recadre dans la zone utile LVGL (440x460) en
// soustrayant les marges boitier et en clampant.
// ============================================================
#include "touch.h"
#include "../../include/pins.h"
#include <Arduino.h>
#include <Wire.h>

// SensorLib est toujours disponible dans ce projet
#include <SensorLib.h>
#include <TouchDrvCSTXXX.hpp>

static TouchDrvCST92XX _drv;
static bool _ok = false;

bool hal::touch_init() {
    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
    if (_drv.begin(Wire, CST9220_SLAVE_ADDRESS, PIN_TP_RST, PIN_TP_INT)) {
        _drv.setSwapXY(false);
        _drv.setMirrorX(false);
        _drv.setMirrorY(false);
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
        int16_t tx, ty;
        if (_drv.getPoint(&tx, &ty, 1) > 0) {
            // Recadrage : coordonnees physiques → zone utile LVGL
            int32_t lx = (int32_t)tx - LCD_MARGIN_H;
            int32_t ly = (int32_t)ty - LCD_MARGIN_V;
            if (lx < 0) lx = 0;
            if (ly < 0) ly = 0;
            if (lx >= LCD_WIDTH)  lx = LCD_WIDTH  - 1;
            if (ly >= LCD_HEIGHT) ly = LCD_HEIGHT - 1;
            x = (uint16_t)lx;
            y = (uint16_t)ly;
            return true;
        }
    }
    return false;
}

bool hal::touch_has_data() {
    return _ok && _drv.isPressed();
}
