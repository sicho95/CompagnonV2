// ============================================================
// ES8311 Codec driver — CompagnonV2
// ============================================================
#include "es8311.h"
#include <Wire.h>
#include <Arduino.h>

static void _write(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(ES8311_ADDR);
    Wire.write(reg); Wire.write(val);
    Wire.endTransmission();
}

uint8_t es8311_read_reg(uint8_t reg) {
    Wire.beginTransmission(ES8311_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)ES8311_ADDR, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0xFF;
}

bool es8311_init(uint32_t sample_rate) {
    Wire.beginTransmission(ES8311_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("[ES8311] not found on I2C 0x18");
        return false;
    }
    _write(ES8311_REG_RESET, 0x1F); delay(10);
    _write(ES8311_REG_RESET, 0x00); delay(10);
    _write(ES8311_REG_CLK_MGR1, 0x30);
    _write(ES8311_REG_CLK_MGR2, 0x00);
    _write(ES8311_REG_CLK_MGR3, 0x10);
    _write(ES8311_REG_CLK_MGR4, 0x10);
    _write(ES8311_REG_CLK_MGR5, 0x00);
    _write(ES8311_REG_CLK_MGR6, 0x00);
    _write(ES8311_REG_CLK_MGR7, 0x08);
    _write(ES8311_REG_CLK_MGR8, 0xFF);
    _write(ES8311_REG_SDPIN,  0x00);
    _write(ES8311_REG_SDPOUT, 0x00);
    _write(ES8311_REG_SYSTEM1, 0x00); _write(ES8311_REG_SYSTEM2, 0x00);
    _write(ES8311_REG_SYSTEM3, 0x10); _write(ES8311_REG_SYSTEM4, 0x10);
    _write(ES8311_REG_SYSTEM5, 0x00); _write(ES8311_REG_SYSTEM6, 0x00);
    _write(ES8311_REG_SYSTEM7, 0x00); _write(ES8311_REG_SYSTEM8, 0xFF);
    _write(ES8311_REG_ADC1,       0x40);
    _write(ES8311_REG_ADC2,       0x00);
    _write(ES8311_REG_ADC_EQ,     0x38);
    _write(ES8311_REG_ADC_VOLUME, 0xBF);
    _write(ES8311_REG_DAC1, 0x00);
    _write(ES8311_REG_DAC2, 0xBF);
    _write(ES8311_REG_GPIO, 0x00);
    _write(ES8311_REG_GP,   0x00);
    Serial.printf("[ES8311] init OK @ %u Hz\n", sample_rate);
    return true;
}

void es8311_set_volume(uint8_t vol)  { _write(ES8311_REG_DAC2, vol); }
void es8311_set_mic_gain(uint8_t g)  { _write(ES8311_REG_ADC_VOLUME, g); }

void es8311_set_mute(bool mute) {
    uint8_t v = es8311_read_reg(ES8311_REG_DAC1);
    if (mute) v |= 0x20; else v &= ~0x20;
    _write(ES8311_REG_DAC1, v);
}
