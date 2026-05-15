// ============================================================
// CompagnonV2 — hal_audio.cpp
// ES7210 4-mic ADC — I2S RX (TDM) uniquement
// NS4150B ampli analogique : pas de I2S TX, juste GPIO46 PA_EN
// Arduino 3.3.8 + LVGL 8.4
// ============================================================
#include "hal_audio.h"
#include "../../../include/pins.h"
#include "../../../lib/es7210/es7210.h"
#include <driver/i2s.h>
#include <Wire.h>

namespace hal {

static bool _initialized = false;
static i2s_port_t _i2s_rx_port = I2S_NUM_1;

// ── PA enable ────────────────────────────────────────────────
static void _pa_enable(bool en) {
    digitalWrite(PIN_SPK_PA_EN, en ? HIGH : LOW);
}

// ── ES7210 I2C init ──────────────────────────────────────────
static bool _es7210_init() {
    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
    // Vérification présence I2C 0x40
    Wire.beginTransmission(0x40);
    if (Wire.endTransmission() != 0) {
        Serial.println("[HAL_AUDIO] ES7210 not found on I2C 0x40");
        return false;
    }
    es7210_adc_init();          // init registres ES7210 (lib es7210)
    es7210_adc_set_gain(ES7210_INPUT_MIC1, GAIN_18DB);
    es7210_adc_set_gain(ES7210_INPUT_MIC2, GAIN_18DB);
    es7210_adc_set_gain(ES7210_INPUT_MIC3, GAIN_18DB);
    es7210_adc_set_gain(ES7210_INPUT_MIC4, GAIN_18DB);
    es7210_adc_codec_enable();
    return true;
}

// ── I2S RX (mic capture TDM) ─────────────────────────────────
static bool _i2s_rx_init() {
    i2s_config_t cfg = {
        .mode                   = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate            = AUDIO_SAMPLE_RATE,
        .bits_per_sample        = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format         = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format   = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags       = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count          = 8,
        .dma_buf_len            = 512,
        .use_apll               = false,
        .tx_desc_auto_clear     = false,
        .fixed_mclk             = 0
    };
    i2s_pin_config_t pins = {
        .mck_io_num   = PIN_ES7210_MCLK,
        .bck_io_num   = PIN_ES7210_BCLK,
        .ws_io_num    = PIN_ES7210_LRCK,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num  = PIN_ES7210_DIN     // GPIO10 ASDOUT
    };
    if (i2s_driver_install(_i2s_rx_port, &cfg, 0, NULL) != ESP_OK) {
        Serial.println("[HAL_AUDIO] I2S RX driver install failed");
        return false;
    }
    i2s_set_pin(_i2s_rx_port, &pins);
    i2s_zero_dma_buffer(_i2s_rx_port);
    return true;
}

// ── Public API ───────────────────────────────────────────────
bool audio_init() {
    if (_initialized) return true;

    // PA_EN pin — démarrer ampli en shutdown
    pinMode(PIN_SPK_PA_EN, OUTPUT);
    _pa_enable(false);

    if (!_es7210_init())  return false;
    if (!_i2s_rx_init())  return false;

    _initialized = true;
    Serial.println("[HAL_AUDIO] ready — I2S RX TDM 16kHz, PA_EN=GPIO46");
    return true;
}

void audio_suspend() {
    _pa_enable(false);
    i2s_stop(_i2s_rx_port);
}

void audio_resume() {
    i2s_start(_i2s_rx_port);
    _pa_enable(true);
}

void audio_pa_enable(bool en) {
    _pa_enable(en);
}

size_t audio_mic_read(int16_t* buf, size_t samples) {
    size_t bytes_read = 0;
    i2s_read(_i2s_rx_port, buf, samples * sizeof(int16_t), &bytes_read, pdMS_TO_TICKS(100));
    return bytes_read / sizeof(int16_t);
}

// audio_spk_write : l'ES7210 est ADC only. Le speaker est piloté
// en analogique par le DAC interne (ligne IN+/IN- vers NS4150B).
// Cette fonction active uniquement le PA pour laisser passer le signal.
void audio_spk_enable(bool en) {
    _pa_enable(en);
}

void audio_set_mic_gain(es7210_input_mic_t mic, es7210_gain_value_t gain) {
    es7210_adc_set_gain(mic, gain);
}

} // namespace hal
