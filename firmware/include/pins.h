// ============================================================
// CompagnonV2 — pins.h
// Waveshare ESP32-S3-Touch-AMOLED-2.16
// Vérifié sur schéma officiel (rev 2) — TOUTES les pages
// Arduino 3.3.8 + LVGL 8.4
// ============================================================
#pragma once

// ─── Display CO5300 (QSPI) ───────────────────────────────────
// Schéma LCD connector J4 :
//   LCD_CS    → GPIO12
//   QSPI_SIO0 → GPIO4
//   QSPI_SI1  → GPIO5
//   QSPI_SI2  → GPIO6
//   QSPI_SI3  → GPIO7
//   QSPI_SCL  → GPIO38  ⚠️ partagé avec ES7210 MCLK
//   LCD_RESET → GPIO39
//   LCD_TE    → non connecté à un GPIO numéroté
#define PIN_LCD_CS      12
#define PIN_LCD_SIO0     4
#define PIN_LCD_SI1      5
#define PIN_LCD_SI2      6
#define PIN_LCD_SI3      7
#define PIN_LCD_SCLK    38   // QSPI_SCL — ⚠️ = ES7210 MCLK, vérifier timing
#define PIN_LCD_RST     39
#define PIN_LCD_TE      -1   // non connecté
#define PIN_LCD_BL      -1   // AMOLED, pas de backlight

// ─── Touch CST9220 (I2C) ─────────────────────────────────────
// Schéma LCD connector J4 :
//   TP_SCL   → GPIO14
//   TP_SDA   → GPIO15
//   TP_INT   → GPIO11
//   TP_RESET → GPIO40
#define PIN_IIC_SDA     15
#define PIN_IIC_SCL     14
#define PIN_TP_INT      11
#define PIN_TP_RST      40

// ─── Audio ADC ES7210 (I2S RX TDM 4-mic) ────────────────────
// Schéma ES7210 U7 :
//   MCLK  → GPIO38  ⚠️ = PIN_LCD_SCLK (QSPI mode, vérifier)
//   SCLK  → GPIO36
//   LRCK  → GPIO35
//   SDOUT1/TDMOUT → 51Ω → GPIO10  (ASDOUT)
// ES7210 I2C addr 0x40 → bus partagé SDA=15, SCL=14
#define PIN_ES7210_MCLK  38
#define PIN_ES7210_BCLK  36
#define PIN_ES7210_LRCK  35
#define PIN_ES7210_DIN   10   // ASDOUT → DIN ESP32 I2S RX

// ─── Audio Speaker NS4150B (ampli analogique Class-D) ────────
// PAS de I2S TX côté ESP32 : le NS4150B reçoit le signal
// analogique directement depuis le DAC interne de l'ES7210.
// GPIO46 = CTRL NS4150B via R16 0R :
//   HIGH → ampli actif
//   LOW  → shutdown (économie énergie)
#define PIN_SPK_PA_EN   46

// ─── IMU QMI8658 (I2C + INT) ─────────────────────────────────
// Schéma (confirmé par l'utilisateur) :
//   INT1 → GPIO17
//   INT2 → GPIO21
// I2C partagé SDA=15, SCL=14
#define PIN_IMU_INT1    17
#define PIN_IMU_INT2    21

// ─── RTC PCF85063 (I2C) ───────────────────────────────────────
// Bus partagé SDA=15, SCL=14

// ─── PMU AXP2101 (I2C + IRQ) ──────────────────────────────────
// I2C addr 0x34 → bus partagé SDA=15, SCL=14
#define PIN_AXP_IRQ      9

// ─── Buttons ──────────────────────────────────────────────────
// Schéma KEYS :
//   Key2 → GPIO0   (BOOT, pull-up 10K, actif bas)
//   Key3 → GPIO18  (bouton utilisateur, pull-up 10K, actif bas)
//   Key1 → PWRON   (géré par AXP2101, pas un GPIO direct)
//   SYS_OUT → GPIO16 (power latch transistor BSS138)
#define PIN_BTN_BOOT     0
#define PIN_BTN_USER    18
#define PIN_SYS_PWR     16   // power latch — HIGH pour maintenir alim

// ─── SD Card (SPI) ────────────────────────────────────────────
// Schéma SD-CARD :
//   MOSI → GPIO1
//   SCK  → GPIO2
//   MISO → GPIO3
//   SDCS → GPIO41
#define PIN_SD_MOSI      1
#define PIN_SD_SCK       2
#define PIN_SD_MISO      3
#define PIN_SD_CS       41

// ─── Wake sources (light sleep) ───────────────────────────────
#define WAKE_GPIO_BTN    PIN_BTN_BOOT   // GPIO0, EXT1
#define WAKE_AXP_IRQ     PIN_AXP_IRQ   // AXP2101 IRQ
// Wake audio possible sur GPIO10 (ES7210 ASDOUT niveau)
