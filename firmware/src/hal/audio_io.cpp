// ============================================================
// CompagnonV2 — HAL Audio I/O — ES8311 I2C + I2S ESP-IDF
// ============================================================
#include "audio_io.h"

// Adresse I2C ES8311
#define ES8311_ADDR   0x18

static bool _audio_ready = false;
static i2s_chan_handle_t _rx_chan = nullptr; // mic
static i2s_chan_handle_t _tx_chan = nullptr; // SPK

// ─── ES8311 I2C init (minimal, sans lib externe) ─────────────────────────────

static bool es8311_write(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(ES8311_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

static bool es8311_init_codec(void) {
    Wire.beginTransmission(ES8311_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("[HAL] Audio ES8311 NOT found on I2C!");
        return false;
    }
    // Reset
    es8311_write(0x00, 0x1F); delay(5);
    es8311_write(0x00, 0x00);
    // Master clock config — MCLK = 256 * Fs (16kHz → 4.096 MHz)
    es8311_write(0x01, 0x30);  // PLL ON, MCLK source = pin
    es8311_write(0x02, 0x00);  // MCLK prescale /1
    es8311_write(0x03, 0x10);  // ADC osr = 32
    es8311_write(0x04, 0x10);  // DAC osr = 32
    // ADC (microphone)
    es8311_write(0x0D, 0x01);  // ADC power up
    es8311_write(0x14, 0x1A);  // ADC gain +26dB
    es8311_write(0x16, 0x24);  // HPF enable
    // DAC (speaker)
    es8311_write(0x31, 0x00);  // DAC power up
    es8311_write(0x32, 0xBF);  // DAC volume max
    es8311_write(0x37, 0x08);  // Class D enable
    Serial.println("[HAL] Audio ES8311 codec init OK");
    return true;
}

// ─── I2S channels (ESP-IDF i2s_std) ──────────────────────────────────────────

static bool i2s_init_channels(void) {
    // Channel config commun
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(
        I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num  = AUDIO_DMA_BUF_COUNT;
    chan_cfg.dma_frame_num = AUDIO_DMA_BUF_LEN;
    chan_cfg.auto_clear    = true;

    esp_err_t err = i2s_new_channel(&chan_cfg, &_tx_chan, &_rx_chan);
    if (err != ESP_OK) {
        Serial.printf("[HAL] I2S channel create FAIL: %s\n", esp_err_to_name(err));
        return false;
    }

    // Config STD 16kHz mono
    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(
                        I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = (gpio_num_t)MIC_MCLK,
            .bclk = (gpio_num_t)MIC_BCLK,
            .ws   = (gpio_num_t)MIC_LRCK,
            .dout = (gpio_num_t)ES8311_DOUT,
            .din  = (gpio_num_t)MIC_DIN,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false }
        }
    };

    err = i2s_channel_init_std_mode(_tx_chan, &std_cfg);
    if (err != ESP_OK) {
        Serial.printf("[HAL] I2S TX init FAIL: %s\n", esp_err_to_name(err));
        return false;
    }
    err = i2s_channel_init_std_mode(_rx_chan, &std_cfg);
    if (err != ESP_OK) {
        Serial.printf("[HAL] I2S RX init FAIL: %s\n", esp_err_to_name(err));
        return false;
    }

    i2s_channel_enable(_tx_chan);
    i2s_channel_enable(_rx_chan);
    Serial.println("[HAL] I2S channels init OK — 16kHz mono 16bit");
    return true;
}

// ─── API publique ─────────────────────────────────────────────────────────────

bool hal_audio_init(void) {
    // PA off pendant init
    pinMode(PA_EN, OUTPUT);
    digitalWrite(PA_EN, LOW);

    if (!es8311_init_codec()) return false;
    if (!i2s_init_channels()) return false;

    _audio_ready = true;
    Serial.println("[HAL] Audio init OK");
    return true;
}

int hal_audio_mic_read(int16_t* buf, size_t nsamples) {
    if (!_audio_ready || !_rx_chan) return -1;
    size_t bytes_read = 0;
    esp_err_t err = i2s_channel_read(_rx_chan, buf,
                                     nsamples * sizeof(int16_t),
                                     &bytes_read, pdMS_TO_TICKS(100));
    if (err != ESP_OK) return -1;
    return (int)(bytes_read / sizeof(int16_t));
}

void hal_audio_spk_write(const int16_t* buf, size_t nsamples) {
    if (!_audio_ready || !_tx_chan) return;
    size_t bytes_written = 0;
    i2s_channel_write(_tx_chan, buf,
                      nsamples * sizeof(int16_t),
                      &bytes_written, pdMS_TO_TICKS(200));
}

void hal_audio_beep(uint16_t freq_hz, uint16_t dur_ms) {
    if (!_audio_ready) return;
    hal_audio_pa_enable(true);
    const size_t   total  = (AUDIO_SAMPLE_RATE * dur_ms) / 1000;
    const uint32_t period = AUDIO_SAMPLE_RATE / freq_hz;
    int16_t* buf = (int16_t*) malloc(total * sizeof(int16_t));
    if (!buf) { hal_audio_pa_enable(false); return; }
    for (size_t i = 0; i < total; i++) {
        buf[i] = (int16_t)((i % period < period / 2) ? 8000 : -8000);
    }
    hal_audio_spk_write(buf, total);
    free(buf);
    delay(dur_ms + 10);
    hal_audio_pa_enable(false);
}

void hal_audio_pa_enable(bool enable) {
    digitalWrite(PA_EN, enable ? HIGH : LOW);
}

bool hal_audio_is_ready(void) {
    return _audio_ready;
}
