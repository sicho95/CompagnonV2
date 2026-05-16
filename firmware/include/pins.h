#pragma once
// ============================================================
// CompagnonV2 — mapping GPIO Waveshare ESP32-S3-Touch-AMOLED-2.16"
// Modifier selon schématique réelle de votre board.
// ============================================================

// ── Display (CO5300 AMOLED via QSPI) ─────────────────────────
#define PIN_QSPI_CS     10
#define PIN_QSPI_SCK    12
#define PIN_QSPI_D0     11
#define PIN_QSPI_D1     13
#define PIN_QSPI_D2     14
#define PIN_QSPI_D3     9
#define PIN_LCD_RST     2
#define PIN_LCD_BL      -1   // AMOLED : pas de rétroéclairage

// ── Touch (CST9220 I2C) ───────────────────────────────────────
#define PIN_IIC_SDA     6
#define PIN_IIC_SCL     7
#define PIN_TP_INT      4
#define PIN_TP_RST      2    // partagé avec LCD_RST

// ── PMU (AXP2101 I2C partagé) ────────────────────────────────
#define PIN_PMU_INT     3

// ── IMU (QMI8658 I2C partagé) ────────────────────────────────
#define PIN_IMU_INT1    44
#define PIN_IMU_INT2    43

// ── Audio (ES7210 ADC + NS4150B ampli) ───────────────────────
// ATTENTION : l'ES7210 n'est PAS présent sur la Waveshare AMOLED 2.16".
// Ces pins sont conservés pour compatibilité avec des shields externes.
#define PIN_ES7210_MCLK  0
#define PIN_ES7210_BCLK  15
#define PIN_ES7210_LRCK  16
#define PIN_ES7210_DIN   17
#define PIN_SPK_PA_EN    46

// ── SD Card (SPI) ─────────────────────────────────────────────
#define PIN_SD_CS        5
#define PIN_SD_MOSI      35
#define PIN_SD_CLK       36
#define PIN_SD_MISO      37

// ── RTC (PCF85063 I2C partagé) ───────────────────────────────
#define PIN_RTC_INT      1

// ── Button ───────────────────────────────────────────────────
#define PIN_BOOT_BTN     0
