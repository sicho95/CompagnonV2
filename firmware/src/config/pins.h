#pragma once
// =============================================================
// CompagnonV2 — pins.h
// Source officielle : Waveshare ESP32-S3-Touch-AMOLED-2.16
// Repo  : https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-2.16
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
//
//  3 boutons physiques sur la carte Waveshare :
//
//  ┌──────────┬────────┬────────────────────────────────────────────────────┐
//  │ Bouton   │ GPIO   │ Actions                                            │
//  ├──────────┼────────┼────────────────────────────────────────────────────┤
//  │ +  (KEY) │ GPIO18 │ Court → tile suivante (carousel RIGHT)             │
//  │          │        │ Long  → lancer l'app sélectionnée                  │
//  ├──────────┼────────┼────────────────────────────────────────────────────┤
//  │ -  (BOOT)│ GPIO0  │ Court → tile précédente (carousel LEFT)            │
//  │          │        │ Long  → retour / quitter l'app active              │
//  ├──────────┼────────┼────────────────────────────────────────────────────┤
//  │ PWR      │ AXP    │ Court (PKEY_SHORT_IRQ) → allumer/éteindre backlight│
//  │          │ I2C    │ Long  (PKEY_LONG_IRQ)  → arrêt complet (poweroff)  │
//  └──────────┴────────┴────────────────────────────────────────────────────┘
//
//  Aucun conflit :
//  - Touch CST816S : INT sur GPIO11, I2C — indépendant
//  - IMU QMI8658   : I2C uniquement, pas de GPIO dédié
//  - PWR géré par AXP2101 via IRQ I2C (pas de GPIO direct)
//
#define PIN_BTN_PLUS     18   // Bouton + (KEY)  — INPUT_PULLUP
#define PIN_BTN_MINUS     0   // Bouton - (BOOT) — INPUT_PULLUP
// PWR : géré via pmu_handle_irq() (PKEY_SHORT_IRQ / PKEY_LONG_IRQ)

// ── I2C ADDRESSES ──────────────────────────────────────────────
#define I2C_ADDR_AXP2101   0x34
#define I2C_ADDR_CST816S   0x15
#define I2C_ADDR_PCF85063  0x51
#define I2C_ADDR_QMI8658   0x6B   // IMU (optionnel)
#define I2C_ADDR_ES7210    0x40
#define I2C_ADDR_ES8311    0x18

// ── PMU chip ───────────────────────────────────────────────────
#define XPOWERS_CHIP_AXP2101   // requis par XPowersLib
