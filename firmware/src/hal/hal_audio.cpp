// ============================================================
// CompagnonV2 — hal_audio.cpp
// ES7210 (0x40) I2S_NUM_1 RX capture mic TDM
// ES8311 (0x18) I2S_NUM_0 TX DAC playback TTS
// NS4150B PA_EN=GPIO46
// fix: supprimer Wire.begin() redondant — Wire est déjà initialisé
//      par pmu.begin() (XPowersLib). Double Wire.begin() génère le
//      warning "Bus already started in Master Mode".
// fix: migrer vers i2s_std.h + dma_desc_num/dma_frame_num (ESP-IDF 5.x)
// ============================================================
#include "hal_audio.h"
#include "drivers/es7210.h"
#include "drivers/es8311.h"
#include <driver/i2s_std.h>
#include <Wire.h>
#include <Arduino.h>

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
  #define PIN_ES7210_DIN   38
#endif
#ifndef PIN_SPK_PA_EN
  #define PIN_SPK_PA_EN    46
#endif

namespace hal {

static bool               _initialized    = false;
static bool               _tx_initialized = false;
static bool               _rx_initialized = false;
static i2s_chan_handle_t  _tx_chan = nullptr;
static i2s_chan_handle_t  _rx_chan = nullptr;

static void _pa_enable(bool en) {
    digitalWrite(PIN_SPK_PA_EN, en ? HIGH : LOW);
}

static bool _i2s_tx_init() {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(
        I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num  = 8;
    chan_cfg.dma_frame_num = 512;
    if (i2s_new_channel(&chan_cfg, &_tx_chan, nullptr) != ESP_OK) {
        Serial.println("[HAL_AUDIO] I2S TX new_channel failed");
        return false;
    }
    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_TTS_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(
                        I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = (gpio_num_t)PIN_I2S_MCLK,
            .bclk = (gpio_num_t)PIN_I2S_SCLK,
            .ws   = (gpio_num_t)PIN_I2S_LRCK,
            .dout = (gpio_num_t)PIN_ES8311_DOUT,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    if (i2s_channel_init_std_mode(_tx_chan, &std_cfg) != ESP_OK) {
        Serial.println("[HAL_AUDIO] I2S TX init_std_mode failed");
        return false;
    }
    i2s_channel_enable(_tx_chan);
    _tx_initialized = true;
    Serial.println("[HAL_AUDIO] I2S TX ready (ES8311 GPIO8)");
    return true;
}

static bool _i2s_rx_init() {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(
        I2S_NUM_1, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num  = 8;
    chan_cfg.dma_frame_num = 512;
    if (i2s_new_channel(&chan_cfg, nullptr, &_rx_chan) != ESP_OK) {
        Serial.println("[HAL_AUDIO] I2S RX new_channel failed");
        return false;
    }
    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(
                        I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = (gpio_num_t)PIN_I2S_MCLK,
            .bclk = (gpio_num_t)PIN_I2S_SCLK,
            .ws   = (gpio_num_t)PIN_I2S_LRCK,
            .dout = I2S_GPIO_UNUSED,
            .din  = (gpio_num_t)PIN_ES7210_DIN,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    if (i2s_channel_init_std_mode(_rx_chan, &std_cfg) != ESP_OK) {
        Serial.println("[HAL_AUDIO] I2S RX init_std_mode failed");
        return false;
    }
    i2s_channel_enable(_rx_chan);
    _rx_initialized = true;
    Serial.println("[HAL_AUDIO] I2S RX ready (ES7210 GPIO38)");
    return true;
}

bool audio_init() {
    if (_initialized) return true;
    pinMode(PIN_SPK_PA_EN, OUTPUT);
    _pa_enable(false);
    // Wire déjà initialisé par pmu.begin() — pas de Wire.begin() ici

    bool es8311_ok = es8311_init(AUDIO_TTS_RATE);
    if (!es8311_ok) Serial.println("[HAL_AUDIO] ES8311 absent");

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
        Serial.println("[HAL_AUDIO] ES7210 absent");
    }

    if (es8311_ok) _i2s_tx_init();
    if (es7210_ok) _i2s_rx_init();

    _initialized = true;
    Serial.printf("[HAL_AUDIO] ready — ES8311:%s ES7210:%s\n",
        es8311_ok ? "OK" : "absent", es7210_ok ? "OK" : "absent");
    return true;
}

void audio_suspend() {
    _pa_enable(false);
    if (_rx_initialized) i2s_channel_disable(_rx_chan);
    if (_tx_initialized) i2s_channel_disable(_tx_chan);
    es8311_set_mute(true);
}

void audio_resume() {
    es8311_set_mute(false);
    if (_rx_initialized) i2s_channel_enable(_rx_chan);
    if (_tx_initialized) i2s_channel_enable(_tx_chan);
}

void audio_pa_enable(bool en)  { _pa_enable(en); }
void audio_spk_enable(bool en) { _pa_enable(en); }
void audio_set_volume(uint8_t vol) { es8311_set_volume(vol); }

size_t audio_mic_read(int16_t* buf, size_t samples) {
    if (!_rx_initialized || !_rx_chan) return 0;
    size_t bytes_read = 0;
    i2s_channel_read(_rx_chan, buf, samples * sizeof(int16_t),
                     &bytes_read, pdMS_TO_TICKS(100));
    return bytes_read / sizeof(int16_t);
}

void audio_set_mic_gain(es7210_input_mic_t mic, es7210_gain_value_t gain) {
    es7210_adc_set_gain(mic, gain);
}

void audio_play_pcm(const uint8_t* buf, size_t len) {
    if (!_tx_initialized || !_tx_chan || !buf || len == 0) return;
    const uint8_t* pcm    = buf;
    size_t         pcmLen = len;
    if (len > 44 && buf[0]=='R' && buf[1]=='I' && buf[2]=='F' && buf[3]=='F') {
        pcm = buf + 44; pcmLen = len - 44;
    }
    _pa_enable(true);
    es8311_set_mute(false);
    size_t offset = 0;
    while (offset < pcmLen) {
        size_t chunk = (pcmLen - offset > 4096) ? 4096 : (pcmLen - offset);
        size_t written = 0;
        i2s_channel_write(_tx_chan, pcm + offset, chunk,
                          &written, pdMS_TO_TICKS(500));
        offset += chunk;
    }
    // vider le DMA avant de couper le PA
    i2s_channel_disable(_tx_chan);
    i2s_channel_enable(_tx_chan);
    delay(80);
    _pa_enable(false);
    es8311_set_mute(true);
}

} // namespace hal
