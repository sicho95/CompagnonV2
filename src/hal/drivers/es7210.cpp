// ============================================================
// ES7210 ADC 4-mic driver — CompagnonV2
// Référence : ES7210 datasheet + ESP-ADF components/es7210
// ============================================================
#include "es7210.h"
#include <Wire.h>
#include <Arduino.h>

#define ES7210_I2C_ADDR  ES7210_ADDR_00

static void _write(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(ES7210_I2C_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

static uint8_t _read(uint8_t reg) {
    Wire.beginTransmission(ES7210_I2C_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)ES7210_I2C_ADDR, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0xFF;
}

void es7210_adc_init() {
    // Reset
    _write(ES7210_REG_RESET, 0xFF);
    delay(10);
    _write(ES7210_REG_RESET, 0x41);  // Normal operation, I2S master

    // Clock : MCLK=12.288MHz, LRCK=16kHz
    _write(ES7210_REG_MCLK_DIV, 0x00);  // MCLK direct
    _write(ES7210_REG_OSR,      0x20);  // OSR = 32
    _write(ES7210_REG_CLK_CTRL, 0x01);  // PLL off, MCLK on

    // Format : TDM 4ch, 16-bit, I2S
    _write(ES7210_REG_SDP_FMT,  0x00);  // I2S standard
    _write(ES7210_REG_SDP_CTRL, 0x01);  // TDM, 4 slots

    // Bias microphone
    _write(ES7210_REG_MIC12_BIAS, 0xAA);
    _write(ES7210_REG_MIC34_BIAS, 0xAA);
    _write(ES7210_REG_ANA_CTRL,   0x0B);

    // Gain initial 18dB sur tous les mics
    _write(ES7210_REG_MIC12_GAIN, GAIN_18DB | (GAIN_18DB >> 4));
    _write(ES7210_REG_MIC34_GAIN, GAIN_18DB | (GAIN_18DB >> 4));

    // Low-power mic off
    _write(ES7210_REG_MIC1_LP, 0x00);
    _write(ES7210_REG_MIC2_LP, 0x00);
    _write(ES7210_REG_MIC3_LP, 0x00);
    _write(ES7210_REG_MIC4_LP, 0x00);

    Serial.println("[ES7210] init OK");
}

void es7210_adc_codec_enable() {
    _write(ES7210_REG_POWER, 0x00);  // power up all ADCs
    delay(5);
    Serial.println("[ES7210] ADC enabled");
}

void es7210_adc_set_gain(es7210_input_mic_t mic, es7210_gain_value_t gain) {
    uint8_t reg, cur;
    if (mic == ES7210_INPUT_MIC1 || mic == ES7210_INPUT_MIC2) {
        reg = ES7210_REG_MIC12_GAIN;
        cur = _read(reg);
        if (mic == ES7210_INPUT_MIC1)
            cur = (cur & 0x0F) | (gain & 0xF0);
        else
            cur = (cur & 0xF0) | (gain >> 4);
    } else {
        reg = ES7210_REG_MIC34_GAIN;
        cur = _read(reg);
        if (mic == ES7210_INPUT_MIC3)
            cur = (cur & 0x0F) | (gain & 0xF0);
        else
            cur = (cur & 0xF0) | (gain >> 4);
    }
    _write(reg, cur);
}
