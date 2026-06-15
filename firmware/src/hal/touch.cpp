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

static constexpr uint8_t  CST92XX_ADDR = 0x5A;
static constexpr uint16_t CST92XX_READ_COMMAND = 0xD000;
static constexpr uint8_t  CST92XX_ACK = 0xAB;
static constexpr uint8_t  CST92XX_MAX_FINGER_NUM = 5;

static void IRAM_ATTR _touch_irq_isr() {
    _irq_seen = true;
}

static int32_t _clamp_i32(int32_t v, int32_t lo, int32_t hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void _map_touch_to_lvgl(int32_t raw_x, int32_t raw_y,
                               uint16_t& x, uint16_t& y) {
    // Match official Waveshare BSP: swap_xy=1, mirror_x=0, mirror_y=1.
    int32_t tx = raw_y;
    int32_t ty = 480 - raw_x;
    int32_t lx = tx - LCD_MARGIN_H;
    int32_t ly = ty - LCD_MARGIN_V;
    x = (uint16_t)_clamp_i32(lx, 0, LCD_WIDTH  - 1);
    y = (uint16_t)_clamp_i32(ly, 0, LCD_HEIGHT - 1);
}

static bool _i2c_write(const uint8_t* data, size_t len) {
    Wire.beginTransmission(CST92XX_ADDR);
    Wire.write(data, len);
    return Wire.endTransmission() == 0;
}

static bool _i2c_read_after_write(const uint8_t* cmd, size_t cmd_len,
                                  uint8_t* out, size_t out_len) {
    Wire.beginTransmission(CST92XX_ADDR);
    Wire.write(cmd, cmd_len);
    if (Wire.endTransmission(false) != 0) return false;

    size_t n = Wire.requestFrom((int)CST92XX_ADDR, (int)out_len);
    if (n != out_len) {
        while (Wire.available()) Wire.read();
        return false;
    }
    for (size_t i = 0; i < out_len; i++) {
        out[i] = (uint8_t)Wire.read();
    }
    return true;
}

static bool _read_raw_point(uint16_t& raw_x, uint16_t& raw_y, uint8_t& event) {
    uint8_t buf[CST92XX_MAX_FINGER_NUM * 5 + 5] = {};
    uint8_t cmd[2] = {
        (uint8_t)(CST92XX_READ_COMMAND >> 8),
        (uint8_t)(CST92XX_READ_COMMAND & 0xFF)
    };

    if (!_i2c_read_after_write(cmd, sizeof(cmd), buf, sizeof(buf))) {
        static uint32_t last_i2c_log = 0;
        if (millis() - last_i2c_log > 1000) {
            Serial.println("[TOUCH] raw i2c read failed");
            last_i2c_log = millis();
        }
        return false;
    }

    uint8_t ack[3] = { cmd[0], cmd[1], CST92XX_ACK };
    if (!_i2c_write(ack, sizeof(ack))) {
        static uint32_t last_ack_log = 0;
        if (millis() - last_ack_log > 1000) {
            Serial.println("[TOUCH] raw ack write failed");
            last_ack_log = millis();
        }
        return false;
    }

    uint8_t num_points = buf[5] & 0x7F;
    if (buf[6] != CST92XX_ACK || num_points == 0 || num_points > CST92XX_MAX_FINGER_NUM) {
        static uint32_t last_empty_log = 0;
        if (millis() - last_empty_log > 1000) {
            Serial.printf("[TOUCH] raw empty b0=%02X b4=%02X pts=%u ack=%02X int=%d\n",
                          buf[0], buf[4], num_points, buf[6], digitalRead(PIN_TP_INT));
            last_empty_log = millis();
        }
        return false;
    }

    uint8_t* p = buf;
    uint8_t id = p[0] >> 4;
    event = p[0] & 0x0F;
    raw_x = ((uint16_t)p[1] << 4) | (p[3] >> 4);
    raw_y = ((uint16_t)p[2] << 4) | (p[3] & 0x0F);
    return id < CST92XX_MAX_FINGER_NUM && event != 0x00;
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
        _drv.sleep();
        _drv.reset();
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
    uint16_t raw_x = 0;
    uint16_t raw_y = 0;
    uint8_t event = 0;
    if (_read_raw_point(raw_x, raw_y, event)) {
        _map_touch_to_lvgl(raw_x, raw_y, x, y);
        _last_x = x;
        _last_y = y;
        _hold_until_ms = now + 80;

        static uint32_t last_raw_log = 0;
        if (now - last_raw_log > 200) {
            Serial.printf("[TOUCH] raw=%u,%u event=%u -> lv=%u,%u int=%d\n",
                          raw_x, raw_y, event, x, y,
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
