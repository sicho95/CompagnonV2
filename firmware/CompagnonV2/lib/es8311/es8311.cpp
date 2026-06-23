// ============================================================
// ES8311 Codec driver — CompagnonV2
// Référence : ES8311 datasheet v1.7 + esp-adf es8311.c
// ============================================================
#include "es8311.h"
#include <Wire.h>
#include <Arduino.h>

// I2C SDA=GPIO15, SCL=GPIO14 (partagé avec ES7210 et AXP2101)
#define ES8311_I2C_SDA  15
#define ES8311_I2C_SCL  14

static void _write(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(ES8311_ADDR);
    Wire.write(reg);
    Wire.write(val);
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
    // Vérifier présence I2C
    Wire.beginTransmission(ES8311_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("[ES8311] not found on I2C 0x18");
        return false;
    }
    Serial.println("[ES8311] found, init...");

    // Reset
    _write(ES8311_REG_RESET, 0x1F);
    delay(10);
    _write(ES8311_REG_RESET, 0x00);
    delay(10);

    // ── Clock manager ──────────────────────────────────────────
    // MCLK source = GPIO42 (external), pas d'inversion
    _write(ES8311_REG_CLK_MGR1, 0x30);  // MCLK from pin, no div
    // MCLK = 12.288 MHz pour 24kHz * 512
    // ADC/DAC CLK div : pour 24kHz LRCK avec MCLK 12.288MHz
    // SCLK = 64*LRCK = 1.536MHz → MCLK/SCLK div = 8
    _write(ES8311_REG_CLK_MGR2, 0x00);  // no MCLK pre-div
    _write(ES8311_REG_CLK_MGR3, 0x10);  // ADC osr = 32
    _write(ES8311_REG_CLK_MGR4, 0x10);  // DAC osr = 32
    // SCLK div : MCLK/SCLK = 8 → div=8
    _write(ES8311_REG_CLK_MGR5, 0x00);
    // LRCK divider MSB/LSB : MCLK/(64*LRCK) = 12288000/(64*24000) = 8
    _write(ES8311_REG_CLK_MGR6, 0x00);  // LRCK div MSB
    _write(ES8311_REG_CLK_MGR7, 0x08);  // LRCK div = 8
    // Tri-state off
    _write(ES8311_REG_CLK_MGR8, 0xFF);

    // ── Format I2S ─────────────────────────────────────────────
    // SDP in (depuis ESP32 → DAC) : I2S standard 16-bit
    _write(ES8311_REG_SDPIN,  0x00);  // I2S, 16bit
    // SDP out (ADC → ESP32) : I2S standard 16-bit
    _write(ES8311_REG_SDPOUT, 0x00);  // I2S, 16bit

    // ── System ─────────────────────────────────────────────────
    _write(ES8311_REG_SYSTEM1, 0x00);
    _write(ES8311_REG_SYSTEM2, 0x00);
    _write(ES8311_REG_SYSTEM3, 0x10);  // vmid enable
    _write(ES8311_REG_SYSTEM4, 0x10);
    _write(ES8311_REG_SYSTEM5, 0x00);
    _write(ES8311_REG_SYSTEM6, 0x00);
    _write(ES8311_REG_SYSTEM7, 0x00);
    _write(ES8311_REG_SYSTEM8, 0xFF);

    // ── ADC (mic interne ES8311 — MIC1P/DMIC_SDA) ─────────────
    _write(ES8311_REG_ADC1,       0x40);  // ADC enable
    _write(ES8311_REG_ADC2,       0x00);
    _write(ES8311_REG_ADC_EQ,     0x38);
    _write(ES8311_REG_ADC_VOLUME, 0xBF);  // 0dB ADC

    // ── DAC ────────────────────────────────────────────────────
    _write(ES8311_REG_DAC1, 0x00);  // DAC enable, pas de soft-mute
    _write(ES8311_REG_DAC2, 0xBF);  // volume 0dB

    // GPIO/analog
    _write(ES8311_REG_GPIO, 0x00);
    _write(ES8311_REG_GP,   0x00);

    Serial.printf("[ES8311] init OK @ %u Hz\n", sample_rate);
    return true;
}

void es8311_set_volume(uint8_t vol) {
    // Registre 0x32 : 0x00 = mute, 0xBF = 0dB, 0xFF = +32dB
    _write(ES8311_REG_DAC2, vol);
}

void es8311_set_mute(bool mute) {
    uint8_t v = es8311_read_reg(ES8311_REG_DAC1);
    if (mute) v |=  0x20;
    else      v &= ~0x20;
    _write(ES8311_REG_DAC1, v);
}

void es8311_set_mic_gain(uint8_t gain) {
    _write(ES8311_REG_ADC_VOLUME, gain);
}
