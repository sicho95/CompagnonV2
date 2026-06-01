// ============================================================
// CompagnonV2 — hal/touch.cpp
// CST9220 touch driver via SensorLib
// Implements hal::touch_init(), hal::touch_read(), hal::touch_has_data()
// ============================================================
#include "touch.h"
#include "../../include/pins.h"
#include <Arduino.h>
#include <Wire.h>

#ifdef USE_SENSORLIB
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
        Serial.printf("[TOUCH] CST9220 init failed (RST=%d INT=%d)\n", PIN_TP_RST, PIN_TP_INT);
    }
    return _ok;
}

bool hal::touch_read(uint16_t& x, uint16_t& y) {
    if (!_ok) return false;
    if (_drv.isPressed()) {
        int16_t tx, ty;
        uint8_t cnt = _drv.getPoint(&tx, &ty, 1);
        if (cnt > 0) {
            x = (uint16_t)tx;
            y = (uint16_t)ty;
            return true;
        }
    }
    return false;
}

bool hal::touch_has_data() {
    return _ok && _drv.isPressed();
}

#else
// SensorLib absent — stubs namespace hal
bool hal::touch_init() {
    Serial.println("[TOUCH] SensorLib absent — stub");
    return false;
}
bool hal::touch_read(uint16_t&, uint16_t&) { return false; }
bool hal::touch_has_data() { return false; }
#endif
