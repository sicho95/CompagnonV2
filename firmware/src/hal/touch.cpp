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
static uint32_t _last_poll_ms = 0;
static hal::TouchFrame _frame = {};
static int16_t _raw_x[hal::TOUCH_MAX_POINTS] = {0};
static int16_t _raw_y[hal::TOUCH_MAX_POINTS] = {0};

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
    // Reprend la logique du BSP Waveshare:
    // flags = { mirror_y = 1, swap_xy = 1 } pour la dalle en rotation courante.
    int32_t tx = sensor_x;
    int32_t ty = (LCD_HEIGHT_PHYS - 1) - sensor_y;

    const int32_t swapped_x = ty;
    const int32_t swapped_y = tx;

    int32_t lx = swapped_x - LCD_MARGIN_H;
    int32_t ly = swapped_y - LCD_MARGIN_V;
    x = (uint16_t)_clamp_i32(lx, 0, LCD_WIDTH  - 1);
    y = (uint16_t)_clamp_i32(ly, 0, LCD_HEIGHT - 1);
}

static void _clear_frame_points() {
    for (uint8_t i = 0; i < hal::TOUCH_MAX_POINTS; ++i) {
        _frame.points[i] = {};
    }
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
        _drv.setSwapXY(false);
        _drv.setMirrorXY(false, false);
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

bool hal::touch_update() {
    if (!_ok) return false;

    uint32_t now = millis();
    bool irq_active = (digitalRead(PIN_TP_INT) == LOW);
    bool poll_due = (now - _last_poll_ms) >= 12;
    bool should_read = _irq_seen || irq_active || poll_due;

    static uint32_t last_idle_log = 0;
    if (!should_read) {
        if (now - last_idle_log > 2000) {
            Serial.printf("[TOUCH] idle irq=%d flag=%d\n",
                          digitalRead(PIN_TP_INT), _irq_seen ? 1 : 0);
            last_idle_log = now;
        }
        return _frame.pressed;
    }

    _irq_seen = false;
    _last_poll_ms = now;
    const uint8_t max_points = (uint8_t)((_drv.getSupportTouchPoint() < hal::TOUCH_MAX_POINTS) ?
                                         _drv.getSupportTouchPoint() : hal::TOUCH_MAX_POINTS);
    uint8_t count = _drv.getPoint(_raw_x, _raw_y, max_points);

    bool was_pressed = _frame.pressed;
    _frame.just_pressed = false;
    _frame.just_released = false;
    _frame.timestamp_ms = now;
    _clear_frame_points();

    if (count > 0) {
        _frame.pressed = true;
        _frame.point_count = count;
        _frame.just_pressed = !was_pressed;

        for (uint8_t i = 0; i < count && i < hal::TOUCH_MAX_POINTS; ++i) {
            uint16_t x = 0;
            uint16_t y = 0;
            _map_touch_to_lvgl(_raw_x[i], _raw_y[i], x, y);
            _frame.points[i].valid = true;
            _frame.points[i].raw_x = _raw_x[i];
            _frame.points[i].raw_y = _raw_y[i];
            _frame.points[i].x = x;
            _frame.points[i].y = y;
        }

        static uint32_t last_raw_log = 0;
        if (now - last_raw_log > 200) {
            Serial.printf("[TOUCH] sensor=%d,%d count=%u -> lv=%u,%u int=%d\n",
                          _frame.points[0].raw_x, _frame.points[0].raw_y,
                          count, _frame.points[0].x, _frame.points[0].y,
                          digitalRead(PIN_TP_INT));
            last_raw_log = now;
        }
        return true;
    }

    _frame.pressed = false;
    _frame.point_count = 0;
    _frame.just_released = was_pressed;
    return false;
}

bool hal::touch_read(uint16_t& x, uint16_t& y) {
    if (!touch_update()) return false;
    if (!_frame.pressed || !_frame.points[0].valid) return false;
    x = _frame.points[0].x;
    y = _frame.points[0].y;
    return true;
}

bool hal::touch_has_data() {
    return _ok && _frame.pressed;
}

const hal::TouchFrame& hal::touch_frame() {
    return _frame;
}
