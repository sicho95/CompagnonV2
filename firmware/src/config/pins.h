#pragma once
// =============================================================
// CompagnonV2 — pins.h
// Source officielle : Waveshare ESP32-S3-Touch-AMOLED-2.16
// Repo  : https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-2.16
// File  : examples/Arduino-v3.3.5/libraries/Mylibrary/pin_config.h
// Toutes ces pins sont valides pour Arduino 3.3.5 ET 3.3.8
//   (même matériel, même PCB, seule la version du core change).
// =============================================================

// ── DISPLAY (CO5300 via QSPI) ──────────────────────────────────
#define PIN_LCD_SDIO0   4
#define PIN_LCD_SDIO1   5
#define PIN_LCD_SDIO2   6
#define PIN_LCD_SDIO3   7
#define PIN_LCD_SCLK   38
#define PIN_LCD_RESET   2
#define PIN_LCD_CS     12
#define LCD_WIDTH     466
#define LCD_HEIGHT    466

// ── I2C BUS (Touch + PMU + RTC + IMU) ─────────────────────────
#define PIN_IIC_SDA    15
#define PIN_IIC_SCL    14

// ── TOUCH (CST816S) ────────────────────────────────────────────
#define PIN_TP_INT     11
#define PIN_TP_RST      2   // partagé avec LCD_RESET (même signal)

// ── AUDIO INPUT : ES7210 (micro I2S) ──────────────────────────
#define PIN_ES7210_BCLK   9
#define PIN_ES7210_LRCK  45
#define PIN_ES7210_DIN   10
#define PIN_ES7210_MCLK  16

// ── AUDIO OUTPUT : ES8311 (DAC I2S) ───────────────────────────
#define PIN_ES8311_DOUT   8
// BCLK/LRCK/MCLK partagés avec ES7210 sur le même bus I2S

// ── AMPLIFICATEUR (PA enable) ─────────────────────────────────
#define PIN_PA           46

// ── BOUTONS ────────────────────────────────────────────────────
#define PIN_BTN_BOOT     0   // BOOT / GPIO0 (INPUT_PULLUP)
// Note : pas de boutons LEFT/RIGHT câblés sur la carte Waveshare,
//        la navigation se fait uniquement via swipe tactile +
//        bouton BOOT (court = action, long = retour).

// ── I2C ADDRESSES ──────────────────────────────────────────────
#define I2C_ADDR_AXP2101   0x34
#define I2C_ADDR_CST816S   0x15
#define I2C_ADDR_PCF85063  0x51
#define I2C_ADDR_QMI8658   0x6B   // IMU (optionnel, non utilisé en v1)
#define I2C_ADDR_ES7210    0x40
#define I2C_ADDR_ES8311    0x18

// ── PMU chip ───────────────────────────────────────────────────
#define XPOWERS_CHIP_AXP2101   // requis par XPowersLib
