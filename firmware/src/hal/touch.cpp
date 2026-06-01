// ============================================================
// CompagnonV2 — hal/touch.cpp
// CST9220 touch driver via SensorLib
// ============================================================
#include "touch.h"
#include <Arduino.h>

#ifdef USE_SENSORLIB
#include <SensorLib.h>
#include <TouchDrvCSTXXX.hpp>

static TouchDrvCST92XX _drv;
static bool _ok = false;

void hal_touch_init(int sda, int scl, int rst, int irq) {
    Wire.begin(sda, scl);
    if (_drv.begin(Wire, CST9220_SLAVE_ADDRESS, rst, irq)) {
        _drv.setSwapXY(false);
        _drv.setMirrorX(false);
        _drv.setMirrorY(false);
        _ok = true;
        Serial.printf("[TOUCH] CST9220 OK — %s (RST=%d INT=%d)\n",
                      _drv.getModelName(), rst, irq);
    } else {
        _ok = false;
        Serial.printf("[TOUCH] CST9220 init failed (RST=%d INT=%d)\n", rst, irq);
    }
}

bool hal_touch_read(int16_t& x, int16_t& y) {
    if (!_ok) return false;
    if (_drv.isPressed()) {
        int16_t tx, ty;
        uint8_t cnt = _drv.getPoint(&tx, &ty, 1);
        if (cnt > 0) { x = tx; y = ty; return true; }
    }
    return false;
}

#else
// SensorLib absent — stub
void hal_touch_init(int, int, int, int) {
    Serial.println("[TOUCH] SensorLib absent — stub");
}
bool hal_touch_read(int16_t&, int16_t&) { return false; }
#endif
