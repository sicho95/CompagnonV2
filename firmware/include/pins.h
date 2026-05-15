// ============================================================
// CompagnonV2 — pins.h
// Waveshare ESP32-S3-Touch-AMOLED-2.16
// Vérifié sur schéma officiel (rev 2)
// Arduino 3.3.8 + LVGL 8.4
// ============================================================
#pragma once

// ─── Display (CO5300, SPI) ───────────────────────────────────
#define PIN_LCD_SCLK   47
#define PIN_LCD_MOSI   18
#define PIN_LCD_CS     45
#define PIN_LCD_DC      4
#define PIN_LCD_RST    -1   // Reset géré par CO5300 en interne
#define PIN_LCD_BL     -1   // Pas de backlight (AMOLED)

// ─── Touch (CST9220, I2C) ────────────────────────────────────
#define PIN_IIC_SDA    15
#define PIN_IIC_SCL    14
#define PIN_TP_RST     13
#define PIN_TP_INT     12

// ─── Audio ADC ES7210 (I2S RX TDM 4-mic) ────────────────────
// Confirmé sur schéma ES7210 (U7) :
//   MCLK  → R38 → GPIO38  (I2S_MCLK)
//   SCLK  → GPIO36        (I2S_SCLK)
//   LRCK  → GPIO35        (I2S_LRCK)
//   SDOUT1/TDMOUT → 51Ω → GPIO10  (I2S_ASDOUT)
// ES7210 I2C addr 0x40 → bus partagé SDA=15, SCL=14
#define PIN_ES7210_MCLK  38
#define PIN_ES7210_BCLK  36
#define PIN_ES7210_LRCK  35
#define PIN_ES7210_DIN   10   // ASDOUT ES7210 → DIN ESP32 (I2S RX data)

// ─── Audio Speaker (NS4150B Class-D, entrée analogique) ──────
// L'ampli NS4150B reçoit le signal depuis le DAC ES7210 en analogique.
// Pas de I2S TX côté ESP32 pour le speaker sur cette carte.
// GPIO46 contrôle le CTRL/shutdown de l'ampli NS4150B :
//   HIGH = ampli actif, LOW = shutdown (économie énergie)
#define PIN_SPK_PA_EN  46   // NS4150B CTRL via R16 0R

// ─── IMU QMI8658 (I2C + INT) ─────────────────────────────────
// Confirmé schéma : INT1=GPIO17, INT2=GPIO21
// I2C partagé SDA=15, SCL=14
#define PIN_IMU_INT1   17
#define PIN_IMU_INT2   21

// ─── RTC PCF85063 (I2C) ──────────────────────────────────────
// Partagé sur le bus I2C (SDA=15, SCL=14)

// ─── PMU AXP2101 (I2C + IRQ) ─────────────────────────────────
#define PIN_AXP_IRQ     9
// AXP2101 I2C addr 0x34 → bus partagé SDA=15, SCL=14

// ─── Buttons ─────────────────────────────────────────────────
#define PIN_BTN_BOOT    0   // BOOT button (GPIO0), wake EXT1
// ⚠️ PIN_BTN_RIGHT : GPIO21 partagé avec IMU INT2 — à confirmer sur schéma
// Si IMU INT2 non utilisé en IRQ, GPIO21 peut servir de bouton
#define PIN_BTN_RIGHT  21

// ─── SD Card (SPI) ───────────────────────────────────────────
// Confirmé sur schéma SD-CARD :
//   MOSI → GPIO1
//   SCK  → GPIO2
//   MISO → GPIO3
//   SDCS → GPIO41  (pas de conflit, IMU INT2 = GPIO21)
#define PIN_SD_MOSI     1
#define PIN_SD_SCK      2
#define PIN_SD_MISO     3
#define PIN_SD_CS      41

// ─── Wake sources (light sleep) ──────────────────────────────
#define WAKE_GPIO_BTN   PIN_BTN_BOOT  // GPIO0, EXT1 wake
#define WAKE_AXP_IRQ    PIN_AXP_IRQ   // AXP2101 IRQ (charger / power key)
// Wake audio : GPIO10 (ES7210 ASDOUT) en niveau si nécessaire
