#include "audio.h"
#include <driver/i2s_std.h>
#include <esp_log.h>

static const char *TAG = "HAL_AUDIO";

// ─── Pins (à ajuster selon schéma Waveshare 2.16") ───────────────────────────
// Microphone I2S (I2S0) — INMP441 ou équivalent
#define I2S_MIC_NUM    I2S_NUM_0
#define I2S_MIC_SCK    12
#define I2S_MIC_WS     13
#define I2S_MIC_DIN    11   // data in

// Codec I2S (I2S1) — NS4168 / MAX98357
#define I2S_SPK_NUM    I2S_NUM_1
#define I2S_SPK_SCK    40
#define I2S_SPK_WS     41
#define I2S_SPK_DOUT   42   // data out

#define SAMPLE_RATE    16000
#define BITS_PER_SAMPLE 16

static i2s_chan_handle_t s_rx_chan = nullptr;
static i2s_chan_handle_t s_tx_chan = nullptr;
static uint8_t          s_volume  = 80;
static bool             s_muted   = false;

void hal_audio_init(void) {
    // ── Microphone RX ─────────────────────────────────────────────────────────
    i2s_chan_config_t rx_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_MIC_NUM, I2S_ROLE_MASTER);
    i2s_new_channel(&rx_cfg, nullptr, &s_rx_chan);

    i2s_std_config_t rx_std = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)I2S_MIC_SCK,
            .ws   = (gpio_num_t)I2S_MIC_WS,
            .dout = I2S_GPIO_UNUSED,
            .din  = (gpio_num_t)I2S_MIC_DIN,
            .invert_flags = { .mclk_inv=false, .bclk_inv=false, .ws_inv=false },
        },
    };
    i2s_channel_init_std_mode(s_rx_chan, &rx_std);
    i2s_channel_enable(s_rx_chan);
    ESP_LOGI(TAG, "Mic I2S init OK (SCK=%d WS=%d DIN=%d)", I2S_MIC_SCK, I2S_MIC_WS, I2S_MIC_DIN);

    // ── Codec TX ──────────────────────────────────────────────────────────────
    i2s_chan_config_t tx_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_SPK_NUM, I2S_ROLE_MASTER);
    i2s_new_channel(&tx_cfg, &s_tx_chan, nullptr);

    i2s_std_config_t tx_std = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)I2S_SPK_SCK,
            .ws   = (gpio_num_t)I2S_SPK_WS,
            .dout = (gpio_num_t)I2S_SPK_DOUT,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = { .mclk_inv=false, .bclk_inv=false, .ws_inv=false },
        },
    };
    i2s_channel_init_std_mode(s_tx_chan, &tx_std);
    i2s_channel_enable(s_tx_chan);
    ESP_LOGI(TAG, "Codec I2S init OK (SCK=%d WS=%d DOUT=%d)", I2S_SPK_SCK, I2S_SPK_WS, I2S_SPK_DOUT);
}

int hal_audio_read(int16_t *buf, size_t samples) {
    size_t bytes_read = 0;
    esp_err_t err = i2s_channel_read(s_rx_chan, buf, samples * sizeof(int16_t),
                                     &bytes_read, pdMS_TO_TICKS(100));
    if (err != ESP_OK) return -1;
    return (int)(bytes_read / sizeof(int16_t));
}

void hal_audio_play(const int16_t *buf, size_t samples) {
    if (s_muted || !s_tx_chan) return;
    // Application du volume par scaling
    int16_t tmp[256];
    size_t  remaining = samples;
    const int16_t *src = buf;
    while (remaining > 0) {
        size_t chunk = remaining > 256 ? 256 : remaining;
        for (size_t i = 0; i < chunk; i++)
            tmp[i] = (int16_t)((int32_t)src[i] * s_volume / 100);
        size_t written = 0;
        i2s_channel_write(s_tx_chan, tmp, chunk * sizeof(int16_t), &written, pdMS_TO_TICKS(100));
        src       += chunk;
        remaining -= chunk;
    }
}

void hal_audio_set_volume(uint8_t vol_pct) {
    s_volume = vol_pct > 100 ? 100 : vol_pct;
}

bool hal_audio_is_muted(void) { return s_muted; }

void hal_audio_mute(bool mute) { s_muted = mute; }
