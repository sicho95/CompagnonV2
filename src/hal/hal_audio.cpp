// ============================================================
// CompagnonV2 — hal_audio.cpp
//
// Architecture audio complète :
//   ES7210 (0x40) ADC 4-mic TDM → I2S_NUM_1 RX  (capture)
//   ES8311 (0x18) Codec DAC     → I2S_NUM_0 TX  (TTS playback)
//   NS4150B PA_EN=GPIO46        → ampli speaker
//
// I2C bus partagé : SDA=GPIO15, SCL=GPIO14
// I2S MCLK=GPIO42, SCLK=GPIO9, LRCK=GPIO45
//   TX DOUT=GPIO8  (ESP32→ES8311 DSDIN)
//   RX DIN =GPIO38 (ES7210 SDOUT1→ESP32)  [à confirmer]
// ============================================================
#include "hal_audio.h"
#include "drivers/es7210.h"
#include "drivers/es8311.h"
#include <driver/i2s.h>
#include <Wire.h>
#include <Arduino.h>
#include <vector>

// ── GPIO pins (définis ici pour Arduino IDE — pas de pins.h externe)
#ifndef PIN_IIC_SDA
  #define PIN_IIC_SDA      15
#endif
#ifndef PIN_IIC_SCL
  #define PIN_IIC_SCL      14
#endif
#ifndef PIN_I2S_MCLK
  #define PIN_I2S_MCLK     42
#endif
#ifndef PIN_I2S_SCLK
  #define PIN_I2S_SCLK      9
#endif
#ifndef PIN_I2S_LRCK
  #define PIN_I2S_LRCK     45
#endif
#ifndef PIN_ES8311_DOUT
  #define PIN_ES8311_DOUT   8
#endif
#ifndef PIN_ES7210_DIN
  #define PIN_ES7210_DIN   38   // à confirmer sur schéma ADC
#endif
#ifndef PIN_SPK_PA_EN
  #define PIN_SPK_PA_EN    46
#endif

namespace hal {

static bool       _initialized    = false;
static bool       _tx_initialized = false;
static bool       _rx_initialized = false;
static i2s_port_t _i2s_rx = I2S_NUM_1;
static i2s_port_t _i2s_tx = I2S_NUM_0;

static void _pa_enable(bool en) {
    digitalWrite(PIN_SPK_PA_EN, en ? HIGH : LOW);
}

static bool _i2s_tx_init() {
    i2s_config_t cfg = {};
    cfg.mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    cfg.sample_rate          = AUDIO_TTS_RATE;
    cfg.bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT;
    cfg.channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count        = 8;
    cfg.dma_buf_len          = 512;
    cfg.use_apll             = false;
    cfg.tx_desc_auto_clear   = true;
    cfg.fixed_mclk           = 0;

    if (i2s_driver_install(_i2s_tx, &cfg, 0, NULL) != ESP_OK) {
        Serial.println("[HAL_AUDIO] I2S TX install failed");
        return false;
    }
    i2s_pin_config_t pins = {};
    pins.mck_io_num   = PIN_I2S_MCLK;
    pins.bck_io_num   = PIN_I2S_SCLK;
    pins.ws_io_num    = PIN_I2S_LRCK;
    pins.data_out_num = PIN_ES8311_DOUT;
    pins.data_in_num  = I2S_PIN_NO_CHANGE;
    i2s_set_pin(_i2s_tx, &pins);
    i2s_zero_dma_buffer(_i2s_tx);
    _tx_initialized = true;
    Serial.println("[HAL_AUDIO] I2S TX ready (ES8311 DAC, 24kHz GPIO8)");
    return true;
}

static bool _i2s_rx_init() {
    i2s_config_t cfg = {};
    cfg.mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
    cfg.sample_rate          = AUDIO_SAMPLE_RATE;
    cfg.bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT;
    cfg.channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count        = 8;
    cfg.dma_buf_len          = 512;
    cfg.use_apll             = false;
    cfg.tx_desc_auto_clear   = false;
    cfg.fixed_mclk           = 0;

    if (i2s_driver_install(_i2s_rx, &cfg, 0, NULL) != ESP_OK) {
        Serial.println("[HAL_AUDIO] I2S RX install failed");
        return false;
    }
    i2s_pin_config_t pins = {};
    pins.mck_io_num   = PIN_I2S_MCLK;
    pins.bck_io_num   = PIN_I2S_SCLK;
    pins.ws_io_num    = PIN_I2S_LRCK;
    pins.data_out_num = I2S_PIN_NO_CHANGE;
    pins.data_in_num  = PIN_ES7210_DIN;
    i2s_set_pin(_i2s_rx, &pins);
    i2s_zero_dma_buffer(_i2s_rx);
    _rx_initialized = true;
    Serial.println("[HAL_AUDIO] I2S RX ready (ES7210 mic TDM, 16kHz GPIO38)");
    return true;
}

bool audio_init() {
    if (_initialized) return true;

    pinMode(PIN_SPK_PA_EN, OUTPUT);
    _pa_enable(false);
    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);

    bool es8311_ok = es8311_init(AUDIO_TTS_RATE);
    if (!es8311_ok)
        Serial.println("[HAL_AUDIO] ES8311 absent — playback désactivé");

    Wire.beginTransmission(ES7210_ADDR_00);
    bool es7210_ok = (Wire.endTransmission() == 0);
    if (es7210_ok) {
        es7210_adc_init();
        es7210_adc_set_gain(ES7210_INPUT_MIC1, GAIN_18DB);
        es7210_adc_set_gain(ES7210_INPUT_MIC2, GAIN_18DB);
        es7210_adc_set_gain(ES7210_INPUT_MIC3, GAIN_18DB);
        es7210_adc_set_gain(ES7210_INPUT_MIC4, GAIN_18DB);
        es7210_adc_codec_enable();
    } else {
        Serial.println("[HAL_AUDIO] ES7210 absent — mic désactivé");
    }

    if (es8311_ok) _i2s_tx_init();
    if (es7210_ok) _i2s_rx_init();

    _initialized = true;
    Serial.printf("[HAL_AUDIO] ready — ES8311:%s ES7210:%s\n",
                  es8311_ok ? "OK" : "absent",
                  es7210_ok ? "OK" : "absent");
    return true;
}

void audio_suspend() {
    _pa_enable(false);
    if (_rx_initialized) i2s_stop(_i2s_rx);
    if (_tx_initialized) i2s_stop(_i2s_tx);
    es8311_set_mute(true);
}

void audio_resume() {
    es8311_set_mute(false);
    if (_rx_initialized) i2s_start(_i2s_rx);
    if (_tx_initialized) i2s_start(_i2s_tx);
}

void audio_pa_enable(bool en)  { _pa_enable(en); }
void audio_spk_enable(bool en) { _pa_enable(en); }
void audio_set_volume(uint8_t vol) { es8311_set_volume(vol); }

size_t audio_mic_read(int16_t* buf, size_t samples) {
    if (!_rx_initialized) return 0;
    size_t bytes_read = 0;
    i2s_read(_i2s_rx, buf, samples * sizeof(int16_t),
             &bytes_read, pdMS_TO_TICKS(100));
    return bytes_read / sizeof(int16_t);
}

void audio_set_mic_gain(es7210_input_mic_t mic, es7210_gain_value_t gain) {
    es7210_adc_set_gain(mic, gain);
}

void audio_play_pcm(const uint8_t* buf, size_t len) {
    if (!_tx_initialized) {
        Serial.println("[HAL_AUDIO] play_pcm: I2S TX non init");
        return;
    }
    if (!buf || len == 0) return;

    const uint8_t* pcm    = buf;
    size_t         pcmLen = len;
    if (len > 44 && buf[0]=='R' && buf[1]=='I' && buf[2]=='F' && buf[3]=='F') {
        pcm    = buf + 44;
        pcmLen = len  - 44;
    }

    _pa_enable(true);
    es8311_set_mute(false);

    size_t offset = 0;
    while (offset < pcmLen) {
        size_t chunk   = (pcmLen - offset > 4096) ? 4096 : (pcmLen - offset);
        size_t written = 0;
        i2s_write(_i2s_tx, pcm + offset, chunk,
                  &written, pdMS_TO_TICKS(500));
        offset += chunk;
    }
    i2s_zero_dma_buffer(_i2s_tx);
    delay(80);
    _pa_enable(false);
    es8311_set_mute(true);
}

} // namespace hal
