// ============================================================
// CompagnonV2 — hal_audio.cpp
// ES7210 4-mic ADC — I2S RX (TDM)
// NS4150B ampli analogique (GPIO46 PA_EN)
// audio_play_pcm : DAC interne ESP32-S3 via I2S TX (I2S_NUM_0)
//   Sortie DAC : GPIO17 (L) / GPIO18 (R) — câbler sur IN+/IN- NS4150B
// ============================================================
#include "hal_audio.h"
#include "../../../include/pins.h"
#include "../../../lib/es7210/es7210.h"
#include <driver/i2s.h>
#include <Wire.h>
#include <Arduino.h>

namespace hal {

static bool        _initialized      = false;
static bool        _tx_initialized   = false;
static i2s_port_t  _i2s_rx_port      = I2S_NUM_1;
static i2s_port_t  _i2s_tx_port      = I2S_NUM_0;  // DAC interne

// ── PA enable ────────────────────────────────────────────────
static void _pa_enable(bool en) {
    digitalWrite(PIN_SPK_PA_EN, en ? HIGH : LOW);
}

// ── ES7210 I2C init ──────────────────────────────────────────
static bool _es7210_init() {
    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
    Wire.beginTransmission(0x40);
    if (Wire.endTransmission() != 0) {
        Serial.println("[HAL_AUDIO] ES7210 not found on I2C 0x40");
        return false;
    }
    es7210_adc_init();
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
        .data_in_num  = PIN_ES7210_DIN
    };
    if (i2s_driver_install(_i2s_rx_port, &cfg, 0, NULL) != ESP_OK) return false;
    i2s_set_pin(_i2s_rx_port, &pins);
    i2s_zero_dma_buffer(_i2s_rx_port);
    return true;
}

// ── I2S TX (DAC interne ESP32-S3) ────────────────────────────
static bool _i2s_tx_init() {
    i2s_config_t cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
        .sample_rate          = 24000,  // Groq TTS output
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 8,
        .dma_buf_len          = 512,
        .use_apll             = false,
        .tx_desc_auto_clear   = true,
        .fixed_mclk           = 0
    };
    if (i2s_driver_install(_i2s_tx_port, &cfg, 0, NULL) != ESP_OK) {
        Serial.println("[HAL_AUDIO] I2S TX driver install failed");
        return false;
    }
    // DAC interne : pas de pin config nécessaire
    i2s_set_dac_mode(I2S_DAC_CHANNEL_LEFT_EN);  // GPIO17
    _tx_initialized = true;
    Serial.println("[HAL_AUDIO] I2S TX DAC ready (GPIO17, 24kHz)");
    return true;
}

// ── Public API ───────────────────────────────────────────────
bool audio_init() {
    if (_initialized) return true;
    pinMode(PIN_SPK_PA_EN, OUTPUT);
    _pa_enable(false);
    if (!_es7210_init())  return false;
    if (!_i2s_rx_init())  return false;
    _i2s_tx_init();  // non bloquant si échec
    _initialized = true;
    Serial.println("[HAL_AUDIO] ready — RX TDM 16kHz + TX DAC 24kHz");
    return true;
}

void audio_suspend() {
    _pa_enable(false);
    i2s_stop(_i2s_rx_port);
    if (_tx_initialized) i2s_stop(_i2s_tx_port);
}

void audio_resume() {
    i2s_start(_i2s_rx_port);
    if (_tx_initialized) i2s_start(_i2s_tx_port);
    _pa_enable(true);
}

void audio_pa_enable(bool en)  { _pa_enable(en); }
void audio_spk_enable(bool en) { _pa_enable(en); }

size_t audio_mic_read(int16_t* buf, size_t samples) {
    size_t bytes_read = 0;
    i2s_read(_i2s_rx_port, buf, samples * sizeof(int16_t),
             &bytes_read, pdMS_TO_TICKS(100));
    return bytes_read / sizeof(int16_t);
}

void audio_set_mic_gain(es7210_input_mic_t mic, es7210_gain_value_t gain) {
    es7210_adc_set_gain(mic, gain);
}

// ── audio_play_pcm — TTS Groq → DAC interne ──────────────────
// buf : WAV brut (skip entête 44 octets) ou PCM nu 16-bit signed
// L'ESP32-S3 DAC attend des données 16-bit UNSIGNED → décalage +32768
void audio_play_pcm(const uint8_t* buf, size_t len) {
    if (!_tx_initialized) {
        if (!_i2s_tx_init()) return;
    }
    if (!buf || len == 0) return;

    // Skip WAV header si présent ("RIFF")
    const uint8_t* pcm = buf;
    size_t pcmLen = len;
    if (len > 44 && buf[0] == 'R' && buf[1] == 'I' && buf[2] == 'F' && buf[3] == 'F') {
        pcm    = buf + 44;
        pcmLen = len - 44;
    }

    // Convertir signed → unsigned pour le DAC interne
    std::vector<uint8_t> dac_buf(pcmLen);
    const int16_t* src = reinterpret_cast<const int16_t*>(pcm);
    int16_t*       dst = reinterpret_cast<int16_t*>(dac_buf.data());
    size_t samples = pcmLen / 2;
    for (size_t i = 0; i < samples; i++) {
        dst[i] = (int16_t)((int32_t)src[i] + 32768) - 32768;
        // DAC interne ESP32 attend MSB des 8 bits haut → shifter
        dst[i] = dst[i] >> 8;
    }
    // DAC repack : 16-bit word, seul les 8 bits hauts comptent
    // Reconstruire en uint16_t avec valeur DAC dans octet haut
    uint16_t* dacWords = reinterpret_cast<uint16_t*>(dac_buf.data());
    for (size_t i = 0; i < samples; i++) {
        uint16_t raw = (uint16_t)((int32_t)src[i] + 32768);
        dacWords[i] = (raw & 0xFF00) | ((raw >> 8) & 0x00FF);
    }

    _pa_enable(true);
    size_t written = 0;
    // Écriture par blocs de 4096 octets
    size_t offset = 0;
    while (offset < pcmLen) {
        size_t chunk = min((size_t)4096, pcmLen - offset);
        i2s_write(_i2s_tx_port,
                  dac_buf.data() + offset, chunk,
                  &written, pdMS_TO_TICKS(500));
        offset += chunk;
    }
    // Attendre la fin de la lecture
    i2s_zero_dma_buffer(_i2s_tx_port);
    delay(100);
    _pa_enable(false);
}

} // namespace hal
