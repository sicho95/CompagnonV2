// ============================================================
// CompagnonV2 — touch.cpp
// CST9220 I2C touch controller
// TP_INT=GPIO11, TP_RST=GPIO40 (corrigé schéma)
// ============================================================
#include <Arduino.h>        // fix: Serial, pinMode, digitalWrite, delay
#include "touch.h"
#include "../../include/pins.h"
#include <Wire.h>

#define CST9220_ADDR  0x1A
#define REG_TOUCH_NUM 0x02
#define REG_TOUCH_X1H 0x03
#define REG_TOUCH_X1L 0x04
#define REG_TOUCH_Y1H 0x05
#define REG_TOUCH_Y1L 0x06

namespace hal {

static bool _touch_ready = false;

bool touch_init() {
    pinMode(PIN_TP_RST, OUTPUT);
    digitalWrite(PIN_TP_RST, LOW);
    delay(10);
    digitalWrite(PIN_TP_RST, HIGH);
    delay(50);

    pinMode(PIN_TP_INT, INPUT);

    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
    Wire.beginTransmission(CST9220_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("[TOUCH] CST9220 not found on I2C 0x1A");
        return false;
    }
    _touch_ready = true;
    Serial.println("[TOUCH] CST9220 ready (INT=GPIO11, RST=GPIO40)");
    return true;
}

bool touch_read(uint16_t &x, uint16_t &y) {
    if (!_touch_ready) return false;
    Wire.beginTransmission(CST9220_ADDR);
    Wire.write(REG_TOUCH_NUM);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)CST9220_ADDR, (uint8_t)5);
    if (Wire.available() < 5) return false;
    uint8_t num  = Wire.read();
    uint8_t xh   = Wire.read();
    uint8_t xl   = Wire.read();
    uint8_t yh   = Wire.read();
    uint8_t yl   = Wire.read();
    if (num == 0) return false;
    x = ((uint16_t)(xh & 0x0F) << 8) | xl;
    y = ((uint16_t)(yh & 0x0F) << 8) | yl;
    return true;
}

bool touch_has_data() {
    return digitalRead(PIN_TP_INT) == LOW;
}

} // namespace hal
