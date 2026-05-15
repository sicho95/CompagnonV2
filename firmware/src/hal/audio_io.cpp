// =============================================================
// CompagnonV2 — hal/audio_io.cpp
// Note : La configuration I2C des codecs ES7210/ES8311 utilise
//        les adresses I2C définies dans pins.h.
//        On n'inclut pas de lib tierce ES7210/ES8311 ici — on
//        passe par les registres I2C directement (compatible
//        Arduino Wire) inspiré des exemples Waveshare 06/07.
// =============================================================
#include "audio_io.h"
#include <Arduino.h>

// ── Helpers I2C register write ────────────────────────────────
static void _i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

// ── ES7210 init (micro input) ─────────────────────────────────
static bool _es7210_init() {
    // Reset
    _i2c_write_reg(I2C_ADDR_ES7210, 0x00, 0xFF);
    delay(10);
    _i2c_write_reg(I2C_ADDR_ES7210, 0x00, 0x41);
    // Mode I2S, 16 bits, MCLK/256
    _i2c_write_reg(I2C_ADDR_ES7210, 0x01, 0x14);
    // Enable MIC1 + MIC2 channels
    _i2c_write_reg(I2C_ADDR_ES7210, 0x07, 0x20);
    _i2c_write_reg(I2C_ADDR_ES7210, 0x08, 0x11);
    // Gain MIC1/MIC2 : 30 dB
    _i2c_write_reg(I2C_ADDR_ES7210, 0x43, 0x1E);
    _i2c_write_reg(I2C_ADDR_ES7210, 0x44, 0x1E);
    Serial.println("[HAL] ES7210 (mic) init OK");
    return true;
}

// ── ES8311 init (DAC output) ──────────────────────────────────
static bool _es8311_init() {
    // Reset
    _i2c_write_reg(I2C_ADDR_ES8311, 0x00, 0x1F);
    delay(10);
    _i2c_write_reg(I2C_ADDR_ES8311, 0x00, 0x00);
    // Clock : MCLK=16MHz/256, SCLK divider
    _i2c_write_reg(I2C_ADDR_ES8311, 0x01, 0x30);
    _i2c_write_reg(I2C_ADDR_ES8311, 0x02, 0x10);
    _i2c_write_reg(I2C_ADDR_ES8311, 0x03, 0x10);
    // Format I2S 16 bits
    _i2c_write_reg(I2C_ADDR_ES8311, 0x0C, 0x0C);
    // Volume DAC : 0 dBFS
    _i2c_write_reg(I2C_ADDR_ES8311, 0x32, 0xBF);
    // Power up
    _i2c_write_reg(I2C_ADDR_ES8311, 0x0D, 0x01);
    Serial.println("[HAL] ES8311 (dac) init OK");
    return true;
}

// ── I2S peripheral init ───────────────────────────────────────
static void _i2s_init() {
    i2s_config_t cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
        .sample_rate          = AUDIO_SAMPLE_RATE,
        .bits_per_sample      = AUDIO_BITS,
        .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = AUDIO_DMA_BUF_COUNT,
        .dma_buf_len          = AUDIO_DMA_BUF_LEN,
        .use_apll             = true,
        .tx_desc_auto_clear   = true,
        .fixed_mclk           = 0,
    };
    i2s_pin_config_t pins = {
        .mck_io_num   = PIN_ES7210_MCLK,
        .bck_io_num   = PIN_ES7210_BCLK,
        .ws_io_num    = PIN_ES7210_LRCK,
        .data_out_num = PIN_ES8311_DOUT,
        .data_in_num  = PIN_ES7210_DIN,
    };
    i2s_driver_install(AUDIO_I2S_PORT, &cfg, 0, NULL);
    i2s_set_pin(AUDIO_I2S_PORT, &pins);
    i2s_set_clk(AUDIO_I2S_PORT, AUDIO_SAMPLE_RATE, AUDIO_BITS, I2S_CHANNEL_STEREO);
}

bool audio_init() {
    pinMode(PIN_PA, OUTPUT);
    digitalWrite(PIN_PA, LOW); // ampli off pendant init

    bool ok = true;
    ok &= _es7210_init();
    ok &= _es8311_init();
    _i2s_init();

    digitalWrite(PIN_PA, HIGH); // ampli on
    Serial.println("[HAL] Audio init OK");
    return ok;
}

void audio_read(int16_t *buf, size_t buf_bytes, size_t *bytes_read) {
    i2s_read(AUDIO_I2S_PORT, buf, buf_bytes, bytes_read, portMAX_DELAY);
}

void audio_write(const int16_t *buf, size_t buf_bytes, size_t *bytes_written) {
    i2s_write(AUDIO_I2S_PORT, buf, buf_bytes, bytes_written, portMAX_DELAY);
}

void audio_set_pa(bool enable) {
    digitalWrite(PIN_PA, enable ? HIGH : LOW);
}

void audio_set_volume(uint8_t vol_pct) {
    // vol_pct 0-100 → registre ES8311 0x32 : 0xFF=+0dB, 0x00=mute
    uint8_t reg_val = (uint8_t)((vol_pct * 0xBF) / 100);
    _i2c_write_reg(I2C_ADDR_ES8311, 0x32, reg_val);
}
