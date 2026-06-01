// ============================================================
// CompagnonV2 — hal/touch.cpp
// CST9220 via SensorLib 0.4.1 — namespace hal
// API 0.4.1 : setMirrorXY(x,y) et getTouchPoints()
// Coords brutes 480x480 recadrées en zone utile LVGL 440x460.
// ============================================================
#include "touch.h"
#include "../../include/pins.h"
#include <Arduino.h>
#include <Wire.h>
#include <SensorLib.h>
#include <TouchDrv.hpp>

static TouchDrvCST92xx _drv;
static bool _ok = false;

bool hal::touch_init() {
    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
    if (_drv.begin(Wire, CST92XX_SLAVE_ADDRESS, PIN_TP_RST, PIN_TP_INT)) {
        _drv.setSwapXY(false);
        _drv.setMirrorXY(false, false);  // SensorLib 0.4.1 : une seule méthode
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
        TouchData td[1];
        uint8_t n = _drv.getTouchPoints(td, 1);  // SensorLib 0.4.1 : getTouchPoints
        if (n > 0) {
            int32_t lx = (int32_t)td[0].x - LCD_MARGIN_H;
            int32_t ly = (int32_t)td[0].y - LCD_MARGIN_V;
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
