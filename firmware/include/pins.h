// ============================================================
// CompagnonV2 — pin_config.h
// Waveshare ESP32-S3-Touch-AMOLED-2.16
// Arduino 3.3.8 + LVGL 8.4
// ============================================================
#pragma once

// ─── Display (CO5300, SPI) ───────────────────────────────────
#define PIN_LCD_SCLK   47
#define PIN_LCD_MOSI   18
#define PIN_LCD_CS     45
#define PIN_LCD_DC     4
#define PIN_LCD_RST    -1
#define PIN_LCD_BL     -1   // No backlight pin (OLED/AMOLED)

// ─── Touch (CST9220, I2C) ────────────────────────────────────
#define PIN_IIC_SDA    15
#define PIN_IIC_SCL    14
#define PIN_TP_RST     13
#define PIN_TP_INT     12

// ─── Audio ADC ES7210 (I2S RX, TDM 4-mic) ───────────────────
#define PIN_ES7210_MCLK  38
#define PIN_ES7210_BCLK  36
#define PIN_ES7210_LRCK  35
#define PIN_ES7210_DIN   37
// ES7210 I2C → shared I2C bus (SDA=15, SCL=14)

// ─── Audio DAC / PA (I2S TX) ─────────────────────────────────
#define PIN_SPK_BCLK   48
#define PIN_SPK_LRCK   26
#define PIN_SPK_DOUT   45   // conflict? verify schematic — tentative
#define PIN_SPK_PA_EN  -1   // PA enable GPIO si présent (à confirmer)

// ─── IMU QMI8658 (I2C) ───────────────────────────────────────
#define PIN_IMU_INT1   40
#define PIN_IMU_INT2   41

// ─── RTC PCF85063 (I2C) ──────────────────────────────────────
// Shared I2C bus

// ─── PMU AXP2101 (I2C + IRQ) ─────────────────────────────────
#define PIN_AXP_IRQ    9

// ─── Buttons ─────────────────────────────────────────────────
#define PIN_BTN_LEFT   0    // BOOT button (GPIO0)
#define PIN_BTN_RIGHT  21

// ─── SD Card (SPI) ────────────────────────────────────────────
#define PIN_SD_SCLK    39
#define PIN_SD_MOSI    11
#define PIN_SD_MISO    13
#define PIN_SD_CS      10

// ─── Wake sources (light sleep) ──────────────────────────────
#define WAKE_GPIO_BTN   PIN_BTN_LEFT   // GPIO0, EXT1 wake
#define WAKE_GPIO_AUD   PIN_ES7210_DIN // GPIO37 audio activity (à valider)
#define WAKE_AXP_IRQ    PIN_AXP_IRQ    // AXP2101 IRQ = charger / power key
