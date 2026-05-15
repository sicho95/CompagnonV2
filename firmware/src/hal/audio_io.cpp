// ============================================================
// CompagnonV2 — hal/audio_io.cpp
// ES8311 codec I2C + I2S
// Basé sur le driver Compagnon2 (xiaozhi heritage)
// ============================================================
#include "audio_io.h"
#include <driver/i2s.h>

// ── Ports I2S ─────────────────────────────────────────────────────────────
#define I2S_PORT_MIC   I2S_NUM_1
#define I2S_PORT_SPK   I2S_NUM_0

// ── Registres ES8311 (I2C) ────────────────────────────────────────────────
static void _es8311_write(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(ES8311_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

static uint8_t _es8311_read(uint8_t reg) {
    Wire.beginTransmission(ES8311_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(ES8311_ADDR, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0xFF;
}

// Séquence init ES8311 pour 16kHz 16bit
static bool _es8311_init() {
    // Vérif présence
    Wire.beginTransmission(ES8311_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("[AUDIO] ERREUR: ES8311 non détecté sur I2C");
        return false;
    }
    // Reset
    _es8311_write(0x00, 0x1F);
    delay(10);
    _es8311_write(0x00, 0x00);
    delay(10);

    // Horloge : MCLK externe 16MHz, LRCK = 16kHz, BCLK = 512kHz
    _es8311_write(0x01, 0x30);  // MCLK source = externe
    _es8311_write(0x02, 0x10);  // PRE-DIV
    _es8311_write(0x03, 0x10);  // DIV
    _es8311_write(0x04, 0x10);
    _es8311_write(0x05, 0x00);
    _es8311_write(0x06, 0x03);  // LRCK div
    _es8311_write(0x07, 0xFF);
    _es8311_write(0x08, 0xFF);

    // Format I2S 16bit
    _es8311_write(0x09, 0x00);
    _es8311_write(0x0A, 0x00);
    _es8311_write(0x10, 0x1F);  // ADC volume
    _es8311_write(0x11, 0x7F);  // ADC PGA gain
    _es8311_write(0x12, 0x00);
    _es8311_write(0x13, 0x10);
    _es8311_write(0x14, 0x00);
    _es8311_write(0x15, 0x00);
    _es8311_write(0x16, 0x24);  // DAC volume
    _es8311_write(0x17, 0x00);

    // Power up ADC + DAC
    _es8311_write(0x0D, 0x01);
    _es8311_write(0x0E, 0x02);
    _es8311_write(0x0F, 0xFF);

    // Activation sortie HP
    _es8311_write(0x1C, 0x6A);
    _es8311_write(0x1D, 0x60);
    _es8311_write(0x1E, 0x00);
    _es8311_write(0x37, 0x08);  // DAC output

    Serial.println("[AUDIO] ES8311 init OK");
    return true;
}

// ── Init I2S MIC ──────────────────────────────────────────────────────────
static bool _i2s_mic_init() {
    i2s_config_t cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate          = AUDIO_SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 4,
        .dma_buf_len          = 512,
        .use_apll             = false,
        .tx_desc_auto_clear   = false,
        .fixed_mclk           = 0
    };
    i2s_pin_config_t pins = {
        .mck_io_num   = MIC_MCLK,
        .bck_io_num   = MIC_BCLK,
        .ws_io_num    = MIC_LRCK,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num  = MIC_DIN
    };
    esp_err_t r = i2s_driver_install(I2S_PORT_MIC, &cfg, 0, nullptr);
    if (r != ESP_OK) { Serial.printf("[AUDIO] I2S MIC install err: %d\n", r); return false; }
    i2s_set_pin(I2S_PORT_MIC, &pins);
    return true;
}

// ── Init I2S SPK ──────────────────────────────────────────────────────────
static bool _i2s_spk_init() {
    i2s_config_t cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate          = AUDIO_SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 4,
        .dma_buf_len          = 512,
        .use_apll             = false,
        .tx_desc_auto_clear   = true,
        .fixed_mclk           = 0
    };
    i2s_pin_config_t pins = {
        .mck_io_num   = I2S_PIN_NO_CHANGE,
        .bck_io_num   = I2S_PIN_NO_CHANGE,
        .ws_io_num    = I2S_PIN_NO_CHANGE,
        .data_out_num = ES8311_DOUT,
        .data_in_num  = I2S_PIN_NO_CHANGE
    };
    esp_err_t r = i2s_driver_install(I2S_PORT_SPK, &cfg, 0, nullptr);
    if (r != ESP_OK) { Serial.printf("[AUDIO] I2S SPK install err: %d\n", r); return false; }
    i2s_set_pin(I2S_PORT_SPK, &pins);
    // Ampli PA
    pinMode(PA_EN, OUTPUT);
    digitalWrite(PA_EN, LOW);  // PA éteint par défaut
    return true;
}

// ── API publique ───────────────────────────────────────────────────────────
bool audio_init() {
    if (!_es8311_init()) return false;
    if (!_i2s_mic_init()) return false;
    if (!_i2s_spk_init()) return false;
    return true;
}

bool audio_mic_start() {
    i2s_start(I2S_PORT_MIC);
    return true;
}

void audio_mic_stop() {
    i2s_stop(I2S_PORT_MIC);
}

int audio_mic_read(int16_t* buf, size_t samples) {
    size_t bytes_read = 0;
    esp_err_t r = i2s_read(I2S_PORT_MIC, buf, samples * sizeof(int16_t), &bytes_read, portMAX_DELAY);
    if (r != ESP_OK) return 0;
    return (int)(bytes_read / sizeof(int16_t));
}

bool audio_spk_start() {
    digitalWrite(PA_EN, HIGH);  // Ampli ON
    i2s_start(I2S_PORT_SPK);
    return true;
}

void audio_spk_stop() {
    i2s_stop(I2S_PORT_SPK);
    delay(10);
    digitalWrite(PA_EN, LOW);   // Ampli OFF
}

bool audio_spk_play(const int16_t* buf, size_t samples) {
    size_t bytes_written = 0;
    esp_err_t r = i2s_write(I2S_PORT_SPK, buf, samples * sizeof(int16_t), &bytes_written, portMAX_DELAY);
    return (r == ESP_OK);
}

void audio_spk_set_volume(uint8_t vol) {
    // Volume via registre DAC ES8311 (0-255 mappe sur 0-100%)
    uint8_t reg_val = (uint8_t)((uint32_t)vol * 255 / 100);
    _es8311_write(0x32, reg_val);
}

void audio_play_beep(uint16_t freq_hz, uint16_t duration_ms) {
    // Génère un beep sinusoïdal simple
    audio_spk_start();
    const uint32_t sample_rate = AUDIO_SAMPLE_RATE;
    const uint32_t total_samples = (uint32_t)sample_rate * duration_ms / 1000;
    const float    omega = 2.0f * M_PI * freq_hz / sample_rate;
    const uint16_t CHUNK = 256;
    int16_t buf[CHUNK];
    uint32_t written = 0;
    while (written < total_samples) {
        uint16_t n = min((uint32_t)CHUNK, total_samples - written);
        for (uint16_t i = 0; i < n; i++) {
            buf[i] = (int16_t)(8000.0f * sinf(omega * (written + i)));
        }
        audio_spk_play(buf, n);
        written += n;
    }
    audio_spk_stop();
}
