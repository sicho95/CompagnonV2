#pragma once
// ============================================================
// CompagnonV2 — GPIO Pin Definitions
// Target: ESP32-S3 + Waveshare AMOLED 2.16" (rm67162)
// ============================================================

// ─── Display (rm67162 QSPI) ────────────────────────
#define PIN_DISP_QSPI_CS    10
#define PIN_DISP_QSPI_SCK   12
#define PIN_DISP_QSPI_D0    11
#define PIN_DISP_QSPI_D1    13
#define PIN_DISP_QSPI_D2    7
#define PIN_DISP_QSPI_D3    17
#define PIN_DISP_RESET      9
#define PIN_DISP_TE         8    // tearing effect (vsync)

// ─── Touch (CST816S I2C) ───────────────────────────
#define PIN_TOUCH_I2C_SDA   6
#define PIN_TOUCH_I2C_SCL   5
#define PIN_TOUCH_INT       4
#define PIN_TOUCH_RST       3

// ─── PMU / Batterie (AXP2101 I2C) ───────────────────
// Partagé avec le bus I2C du touch
#define PIN_PMU_I2C_SDA     6    // même bus I2C
#define PIN_PMU_I2C_SCL     5
#define PIN_PMU_IRQ         38

// ─── Boutons physiques ────────────────────────────
#define PIN_BTN_LEFT        0    // GPIO0 (BOOT)
#define PIN_BTN_RIGHT       14

// ─── Audio I2S (microphone INMP441) ──────────────────
#define PIN_I2S_MIC_WS      41
#define PIN_I2S_MIC_SCK     42
#define PIN_I2S_MIC_SD      2

// ─── Audio I2S (DAC/amp MAX98357 ou PCM5102) ─────────
#define PIN_I2S_DAC_WS      39
#define PIN_I2S_DAC_BCK     40
#define PIN_I2S_DAC_DATA    48

// ─── SD Card (SPI) ──────────────────────────────
#define PIN_SD_CS           46
#define PIN_SD_MOSI         45
#define PIN_SD_SCK          47
#define PIN_SD_MISO         21

// ─── GPS UART (optionnel) ─────────────────────────
#define PIN_GPS_TX          43
#define PIN_GPS_RX          44
#define UART_GPS            Serial2

// ─── LED RGB (WS2812B, 1 LED sur la carte) ───────────
#define PIN_LED_RGB         15
#define LED_COUNT           1
