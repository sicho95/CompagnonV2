// ============================================================
// CompagnonV2 — HAL Audio implementation
// ES7210 (I2S RX TDM) + I2S TX (speaker)
// Arduino 3.3.8 legacy I2S driver
// Porté depuis 06_ES7210.ino (Compagnon2) — adapté namespace hal
// ============================================================
#include "hal_audio.h"
#include "../../include/pins.h"
#include <Arduino.h>
#include <Wire.h>
#include <driver/i2s.h>
#include "../lib/es7210/es7210.h"
#include "../lib/es7210/audio_hal.h"

#define AUDIO_SAMPLE_RATE   16000
#define AUDIO_I2S_MIC_CH    I2S_NUM_1
#define AUDIO_I2S_SPK_CH    I2S_NUM_0

namespace hal {

static bool _initialized = false;

static bool _init_es7210() {
    audio_hal_codec_config_t cfg = {
        .adc_input  = AUDIO_HAL_ADC_INPUT_ALL,
        .codec_mode = AUDIO_HAL_CODEC_MODE_ENCODE,
        .i2s_iface  = {
            .mode    = AUDIO_HAL_MODE_SLAVE,
            .fmt     = AUDIO_HAL_I2S_NORMAL,
            .samples = AUDIO_HAL_16K_SAMPLES,
            .bits    = AUDIO_HAL_BIT_LENGTH_16BITS,
        },
    };
    uint32_t r = ESP_OK;
    r |= es7210_adc_init(&Wire, &cfg);
    r |= es7210_adc_config_i2s(cfg.codec_mode, &cfg.i2s_iface);
    r |= es7210_adc_set_gain(
        (es7210_input_mics_t)(ES7210_INPUT_MIC1 | ES7210_INPUT_MIC2), GAIN_0DB);
    r |= es7210_adc_set_gain(
        (es7210_input_mics_t)(ES7210_INPUT_MIC3 | ES7210_INPUT_MIC4), GAIN_37_5DB);
    r |= es7210_adc_ctrl_state(cfg.codec_mode, AUDIO_HAL_CTRL_START);
    return (r == ESP_OK);
}

static bool _init_i2s_rx() {
    i2s_config_t cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate          = AUDIO_SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_ALL_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 8,
        .dma_buf_len          = 64,
        .use_apll             = false,
        .tx_desc_auto_clear   = true,
        .fixed_mclk           = 0,
        .mclk_multiple        = I2S_MCLK_MULTIPLE_256,
        .bits_per_chan         = I2S_BITS_PER_CHAN_16BIT,
        .chan_mask            = (i2s_channel_t)(I2S_TDM_ACTIVE_CH0 | I2S_TDM_ACTIVE_CH1),
    };
    i2s_pin_config_t pins = {0};
    pins.mck_io_num  = PIN_ES7210_MCLK;
    pins.bck_io_num  = PIN_ES7210_BCLK;
    pins.ws_io_num   = PIN_ES7210_LRCK;
    pins.data_in_num = PIN_ES7210_DIN;
    esp_err_t r = i2s_driver_install(AUDIO_I2S_MIC_CH, &cfg, 0, NULL);
    if (r != ESP_OK) return false;
    r = i2s_set_pin(AUDIO_I2S_MIC_CH, &pins);
    i2s_zero_dma_buffer(AUDIO_I2S_MIC_CH);
    return (r == ESP_OK);
}

static bool _init_i2s_tx() {
    i2s_config_t cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate          = AUDIO_SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 8,
        .dma_buf_len          = 128,
        .use_apll             = false,
        .tx_desc_auto_clear   = true,
    };
    i2s_pin_config_t pins = {0};
    pins.bck_io_num   = PIN_SPK_BCLK;
    pins.ws_io_num    = PIN_SPK_LRCK;
    pins.data_out_num = PIN_SPK_DOUT;
    pins.data_in_num  = I2S_PIN_NO_CHANGE;
    esp_err_t r = i2s_driver_install(AUDIO_I2S_SPK_CH, &cfg, 0, NULL);
    if (r != ESP_OK) return false;
    r = i2s_set_pin(AUDIO_I2S_SPK_CH, &pins);
#if PIN_SPK_PA_EN >= 0
    pinMode(PIN_SPK_PA_EN, OUTPUT);
    digitalWrite(PIN_SPK_PA_EN, HIGH);
#endif
    return (r == ESP_OK);
}

bool audio_init() {
    if (_initialized) return true;
    bool ok = _init_es7210() && _init_i2s_rx() && _init_i2s_tx();
    _initialized = ok;
    return ok;
}

size_t audio_mic_read(int16_t* buf, size_t samples) {
    size_t bytes_read = 0;
    i2s_read(AUDIO_I2S_MIC_CH, buf, samples * sizeof(int16_t), &bytes_read, portMAX_DELAY);
    return bytes_read / sizeof(int16_t);
}

size_t audio_spk_write(const int16_t* buf, size_t samples) {
    size_t written = 0;
    i2s_write(AUDIO_I2S_SPK_CH, buf, samples * sizeof(int16_t), &written, portMAX_DELAY);
    return written / sizeof(int16_t);
}

size_t audio_spk_write_bytes(const uint8_t* buf, size_t len) {
    size_t written = 0;
    i2s_write(AUDIO_I2S_SPK_CH, buf, len, &written, portMAX_DELAY);
    return written;
}

void audio_set_mic_gain(uint8_t gain_db) {
    es7210_gain_value_t g = (gain_db >= 30) ? GAIN_37_5DB :
                            (gain_db >= 15) ? GAIN_15DB   : GAIN_0DB;
    es7210_adc_set_gain(
        (es7210_input_mics_t)(ES7210_INPUT_MIC1 | ES7210_INPUT_MIC2 |
                              ES7210_INPUT_MIC3 | ES7210_INPUT_MIC4), g);
}

void audio_suspend() {
    i2s_zero_dma_buffer(AUDIO_I2S_SPK_CH);
    i2s_stop(AUDIO_I2S_MIC_CH);
    i2s_stop(AUDIO_I2S_SPK_CH);
    es7210_adc_ctrl_state(AUDIO_HAL_CODEC_MODE_ENCODE, AUDIO_HAL_CTRL_STOP);
}

void audio_resume() {
    es7210_adc_ctrl_state(AUDIO_HAL_CODEC_MODE_ENCODE, AUDIO_HAL_CTRL_START);
    i2s_start(AUDIO_I2S_MIC_CH);
    i2s_start(AUDIO_I2S_SPK_CH);
}

} // namespace hal
