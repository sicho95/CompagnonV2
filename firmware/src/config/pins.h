/**
 * pins.h — Pinout Waveshare ESP32-S3-Touch-AMOLED-2.16"
 * CO5300 QSPI / CST9220 I2C / AXP2101 / QMI8658 / PCF85063
 */
#pragma once

// Ecran CO5300 (QSPI)
#define PIN_LCD_CS    10
#define PIN_LCD_SCK   12
#define PIN_LCD_MOSI  11
#define PIN_LCD_MISO  13
#define PIN_LCD_D2    14
#define PIN_LCD_D3    9
#define PIN_LCD_RST   17
#define PIN_LCD_DC    8
#define PIN_LCD_BL    38

// Touch CST9220 (I2C)
#define PIN_TOUCH_SDA  3
#define PIN_TOUCH_SCL  2
#define PIN_TOUCH_INT  1
#define PIN_TOUCH_RST  4
#define TOUCH_I2C_ADDR 0x1A

// PMU AXP2101 (I2C)
#define PIN_PMU_SDA  PIN_TOUCH_SDA
#define PIN_PMU_SCL  PIN_TOUCH_SCL
#define PMU_I2C_ADDR 0x34

// IMU QMI8658
#define IMU_I2C_ADDR 0x6B

// RTC PCF85063
#define RTC_I2C_ADDR 0x51

// Micro MEMS (I2S)
#define PIN_MIC_CLK   42
#define PIN_MIC_DATA  41
#define PIN_MIC_WS    40

// Haut-parleur I2S
#define PIN_SPK_BCLK  39
#define PIN_SPK_LRC   37
#define PIN_SPK_DOUT  36

// SD Card (SPI)
#define PIN_SD_CS    15
#define PIN_SD_SCK   16
#define PIN_SD_MOSI  18
#define PIN_SD_MISO  19

// Boutons physiques
#define PIN_BTN_LEFT   0
#define PIN_BTN_RIGHT  6
