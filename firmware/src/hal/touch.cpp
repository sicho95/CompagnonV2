// ============================================================
// CompagnonV2 — hal/touch.cpp
// CST9220 via SensorLib 0.4.1 — namespace hal
// IMPORTANT : setPins() configure RST/INT pour SensorLib ; begin()
// garde SDA/SCL à -1 car Wire est déjà initialisé ailleurs.
// ============================================================
#include "touch.h"
#include "../../include/pins.h"
#include <Arduino.h>
#include <Wire.h>
#include <SensorLib.h>
#include <TouchDrv.hpp>

static TouchDrvCST92xx _drv;
static bool _ok = false;
static volatile bool _irq_seen = false;
static uint16_t _last_x = 0;
static uint16_t _last_y = 0;
static uint32_t _hold_until_ms = 0;
static uint32_t _last_poll_ms = 0;

static void IRAM_ATTR _touch_irq_isr() {
    _irq_seen = true;
}

static int32_t _clamp_i32(int32_t v, int32_t lo, int32_t hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void _map_touch_to_lvgl(int32_t sensor_x, int32_t sensor_y,
                               uint16_t& x, uint16_t& y) {
    int32_t lx = sensor_x - LCD_MARGIN_H;
    int32_t ly = sensor_y - LCD_MARGIN_V;
    x = (uint16_t)_clamp_i32(lx, 0, LCD_WIDTH  - 1);
    y = (uint16_t)_clamp_i32(ly, 0, LCD_HEIGHT - 1);
}

bool hal::touch_init() {
    pinMode(PIN_TP_INT, INPUT_PULLUP);
    pinMode(PIN_TP_RST, OUTPUT);
    digitalWrite(PIN_TP_RST, LOW);
    delay(30);
    digitalWrite(PIN_TP_RST, HIGH);
    delay(50);
    delay(1000);

    _drv.setPins(PIN_TP_RST, PIN_TP_INT);

    // Wire déjà init par PMU — ne pas rappeler Wire.setPins()/begin().
    if (_drv.begin(Wire, CST92XX_SLAVE_ADDRESS, -1, -1)) {
        pinMode(PIN_TP_INT, INPUT_PULLUP);
        _drv.setMaxCoordinates(480, 480);
        attachInterrupt(digitalPinToInterrupt(PIN_TP_INT), _touch_irq_isr, FALLING);
        _drv.setSwapXY(true);
        _drv.setMirrorXY(true, false);
        _ok = true;
        Serial.printf("[TOUCH] CST9220 OK — %s (RST=%d INT=%d irq=%d)\n",
                      _drv.getModelName(), PIN_TP_RST, PIN_TP_INT,
                      digitalRead(PIN_TP_INT));
    } else {
        _ok = false;
        Serial.printf("[TOUCH] CST9220 init FAILED (RST=%d INT=%d)\n",
                      PIN_TP_RST, PIN_TP_INT);
    }
    return _ok;
}

bool hal::touch_read(uint16_t& x, uint16_t& y) {
    if (!_ok) return false;

    uint32_t now = millis();
    bool irq_active = (digitalRead(PIN_TP_INT) == LOW);
    bool poll_due = (now - _last_poll_ms) >= 12;
    bool should_read = _irq_seen || irq_active || poll_due;

    if (!should_read && now < _hold_until_ms) {
        x = _last_x;
        y = _last_y;
        return true;
    }

    static uint32_t last_idle_log = 0;
    if (!should_read) {
        if (now - last_idle_log > 2000) {
            Serial.printf("[TOUCH] idle irq=%d flag=%d\n",
                          digitalRead(PIN_TP_INT), _irq_seen ? 1 : 0);
            last_idle_log = now;
        }
        return false;
    }

    _irq_seen = false;
    _last_poll_ms = now;
    const TouchPoints& points = _drv.getTouchPoints();
    uint8_t count = points.getPointCount();
    if (count > 0) {
        const TouchPoint& pt = points.getPoint(0);
        _map_touch_to_lvgl(pt.x, pt.y, x, y);
        _last_x = x;
        _last_y = y;
        _hold_until_ms = now + 80;

        static uint32_t last_raw_log = 0;
        if (now - last_raw_log > 200) {
            Serial.printf("[TOUCH] sensor=%d,%d count=%u -> lv=%u,%u int=%d\n",
                          pt.x, pt.y, count, x, y,
                          digitalRead(PIN_TP_INT));
            last_raw_log = now;
        }
        return true;
    }
    _hold_until_ms = 0;
    return false;
}

bool hal::touch_has_data() {
    return _ok && _drv.isPressed();
}
