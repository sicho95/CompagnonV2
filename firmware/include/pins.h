#pragma once
// ============================================================
// CompagnonV2 — GPIO mapping Waveshare ESP32-S3-Touch-AMOLED-2.16"
// Source : schémas matériels + code officiel Waveshare (mai 2026)
// ============================================================

// ── I2C bus partagé : Touch / RTC / PMU / IMU / audio ctrl ──
#define PIN_IIC_SDA       15
#define PIN_IIC_SCL       14

// ── Display CO5300 AMOLED (QSPI via connecteur J4) ───────────
#define PIN_LCD_CS        12
#define PIN_LCD_SCLK      38
#define PIN_LCD_SIO0       4
#define PIN_LCD_SI1        5
#define PIN_LCD_SI2        6
#define PIN_LCD_SI3        7
// NOTE: LCD_RST = 2, partagé avec TP_RST — confirmé Waveshare pin_config.h
#define PIN_LCD_RST        2
#define PIN_LCD_TE         8

// Résolution écran
#define LCD_WIDTH        480
#define LCD_HEIGHT       480

// Aliases historiques
#define PIN_QSPI_CS       PIN_LCD_CS
#define PIN_QSPI_SCK      PIN_LCD_SCLK
#define PIN_QSPI_D0       PIN_LCD_SIO0
#define PIN_QSPI_D1       PIN_LCD_SI1
#define PIN_QSPI_D2       PIN_LCD_SI2
#define PIN_QSPI_D3       PIN_LCD_SI3

// ── Touch CST9220 ─────────────────────────────────────────────
#define PIN_TP_INT        11
// TP_RST partagé avec LCD_RST (GPIO2) — confirmé Waveshare
#define PIN_TP_RST         2

// ── RTC PCF85063 ──────────────────────────────────────────────
#define PIN_RTC_INT       13

// ── PMU AXP2101 ───────────────────────────────────────────────
#define PIN_PMU_INT        3

// ── IMU QMI8658 ───────────────────────────────────────────────
#define PIN_IMU_INT1      17
#define PIN_IMU_INT2      21
#define QMI8658_I2C_ADDR 0x6B

// ── I2S audio ES8311 / ES7210 ─────────────────────────────────
// GPIO16 = MCLK audio (confirmé Waveshare pin_config.h)
// IMPORTANT : GPIO16 n'est PAS SYS_PWR sur ce board
#define PIN_I2S_MCLK      16
#define PIN_I2S_SCLK       9
#define PIN_I2S_LRCK      45
#define PIN_ES8311_DIN     8   // DSDIN
#define PIN_ES8311_DOUT   10   // ASDOUT
#define PIN_ES7210_DIN    10
#define PIN_SPK_PA_EN     46

// Aliases pour hal_audio.cpp
#define PIN_ES7210_MCLK   PIN_I2S_MCLK
#define PIN_ES7210_BCLK   PIN_I2S_SCLK
#define PIN_ES7210_LRCK   PIN_I2S_LRCK

// ── SD Card ───────────────────────────────────────────────────
#define PIN_SD_MOSI        1
#define PIN_SD_CLK         2
#define PIN_SD_MISO        3
#define PIN_SD_CS         41

// ── Boutons / USB / UART ──────────────────────────────────────
#define PIN_BOOT_BTN       0
#define PIN_KEY3          18
#define PIN_USB_DN        19
#define PIN_USB_DP        20
#define PIN_UART_TX       43
#define PIN_UART_RX       44
