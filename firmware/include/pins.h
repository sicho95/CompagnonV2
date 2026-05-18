#pragma once
// ============================================================
// CompagnonV2 — GPIO mapping Waveshare ESP32-S3-Touch-AMOLED-2.16"
// Source : schémas ADC/Codec/Speaker/KEYS fournis
// ============================================================

// ── I2C bus partagé (ES7210 + ES8311 + AXP2101 + QMI8658) ───
#define PIN_IIC_SDA     15
#define PIN_IIC_SCL     14

// ── Display CO5300 AMOLED (QSPI) ────────────────────────────
#define PIN_QSPI_CS      10
#define PIN_QSPI_SCK     12
#define PIN_QSPI_D0      11
#define PIN_QSPI_D1      13
#define PIN_QSPI_D2      14
#define PIN_QSPI_D3       9
#define PIN_LCD_RST       2

// Aliases utilisés par display.cpp
#define PIN_LCD_CS       PIN_QSPI_CS
#define PIN_LCD_SCLK     PIN_QSPI_SCK
#define PIN_LCD_SIO0     PIN_QSPI_D0
#define PIN_LCD_SI1      PIN_QSPI_D1
#define PIN_LCD_SI2      PIN_QSPI_D2
#define PIN_LCD_SI3      PIN_QSPI_D3

// ── Touch CST9220 (I2C partagé) ──────────────────────────────
#define PIN_TP_INT        4
#define PIN_TP_RST        2   // partagé avec LCD_RST

// ── PMU AXP2101 (I2C partagé) ────────────────────────────────
#define PIN_PMU_INT       3

// ── IMU QMI8658 (I2C partagé) ────────────────────────────────
#define PIN_IMU_INT1     44
#define PIN_IMU_INT2     43

// ── I2S bus partagé ES7210(RX) + ES8311(TX/RX) ───────────────
#define PIN_I2S_MCLK     42
#define PIN_I2S_SCLK      9
#define PIN_I2S_LRCK     45
#define PIN_ES8311_DOUT   8
#define PIN_ES8311_DIN    8
#define PIN_ES7210_DIN   38

// Aliases pour hal_audio.cpp
#define PIN_ES7210_MCLK  PIN_I2S_MCLK
#define PIN_ES7210_BCLK  PIN_I2S_SCLK
#define PIN_ES7210_LRCK  PIN_I2S_LRCK

// ── Speaker PA NS4150B ────────────────────────────────────────
#define PIN_SPK_PA_EN    46

// ── Power latch (SYS_PWR) ─────────────────────────────────────
// GPIO16 → Gate BSS138 (T1) via R11 1K → maintient SYS_OUT HIGH
// Confirmé sur schéma KEYS : SYS_OUT connecté à GPIO16
// À mettre HIGH dès le boot pour éviter l'extinction spontanée
#define PIN_SYS_PWR      16

// ── Keys ─────────────────────────────────────────────────────
// Key2 = GPIO0  (boot button, alias PIN_BOOT_BTN)
// Key3 = GPIO18 (bouton utilisateur)
#define PIN_KEY3         18

// ── SD Card (SPI optionnel) ───────────────────────────────────
#define PIN_SD_CS         5
#define PIN_SD_MOSI      35
#define PIN_SD_CLK       36
#define PIN_SD_MISO      37

// ── RTC PCF85063 (I2C partagé) ───────────────────────────────
#define PIN_RTC_INT       1

// ── Boot button ──────────────────────────────────────────────
#define PIN_BOOT_BTN      0
