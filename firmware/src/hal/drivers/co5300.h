#pragma once
// ============================================================
// CompagnonV2 — drivers/co5300.h
// CO5300 AMOLED QSPI driver — Waveshare ESP32-S3-AMOLED-2.16"
// Bus : SPI3 QSPI  CS=10 SCK=12 D0=11 D1=13 D2=14 D3=9
// Résolution : 536 x 240  (landscape)
// Protocole  : commandes 1-line SPI + data 4-line QSPI
// Référence  : LILYGO T4-S3 / Waveshare CO5300 init seq
// ============================================================
#include <Arduino.h>
#include <SPI.h>
#include "../../config/pins.h"

namespace co5300 {

// ── SPI bus ─────────────────────────────────────────────────
static SPIClass _spi(HSPI);
static SPISettings _spi_cfg(40000000UL, MSBFIRST, SPI_MODE0);

// ── Helpers bas niveau ──────────────────────────────────────
static inline void _cs_low()  { digitalWrite(PIN_LCD_CS, LOW);  }
static inline void _cs_high() { digitalWrite(PIN_LCD_CS, HIGH); }

// Envoie 1 octet de commande en mode 1-line SPI (D0 uniquement)
static void _cmd(uint8_t cmd) {
    _cs_low();
    // Byte de contrôle : bit7=0 (commande), bit6=0 (1 ligne), bits5-0=0
    _spi.transfer(0x02);   // write command token CO5300
    _spi.transfer(0x00);
    _spi.transfer(cmd);
    _spi.transfer(0x00);
    _cs_high();
}

// Envoie des data en mode QSPI (4 lignes) — séquence CO5300
static void _dat(const uint8_t* buf, size_t len) {
    _cs_low();
    _spi.transfer(0x32);   // write data token QSPI CO5300
    _spi.transfer(0x00);
    _spi.transfer(0x2C);   // RAMWR continu
    _spi.transfer(0x00);
    _spi.writeBytes(buf, len);
    _cs_high();
}

// ── Séquence d'initialisation ────────────────────────────────
static void _init_seq() {
    // Sleep out
    _cmd(0x11); delay(120);
    // Interface pixel format : 16bpp RGB565
    _cs_low();
    _spi.transfer(0x02); _spi.transfer(0x00); _spi.transfer(0x3A); _spi.transfer(0x00);
    _spi.transfer(0x55); // 16bpp
    _cs_high(); delay(10);
    // Display inversion ON (AMOLED)
    _cmd(0x21); delay(10);
    // QSPI enable (CO5300 vendor cmd)
    _cs_low();
    _spi.transfer(0x02); _spi.transfer(0x00); _spi.transfer(0xC2); _spi.transfer(0x00);
    _spi.transfer(0x02);
    _cs_high(); delay(10);
    // Tearing effect OFF
    _cmd(0x34);
    // Display ON
    _cmd(0x29); delay(20);
}

// ── API publique ─────────────────────────────────────────────

void init() {
    // Reset matériel
    if (PIN_LCD_RST >= 0) {
        pinMode(PIN_LCD_RST, OUTPUT);
        digitalWrite(PIN_LCD_RST, LOW);  delay(10);
        digitalWrite(PIN_LCD_RST, HIGH); delay(120);
    }
    pinMode(PIN_LCD_CS, OUTPUT);
    digitalWrite(PIN_LCD_CS, HIGH);

    // Démarre SPI3 (HSPI) avec les broches QSPI
    // D1/D2/D3 sont configurés en output — le DMA SPI les pilote
    _spi.begin(PIN_LCD_SCLK, PIN_LCD_SI3 /*MISO=D3*/, PIN_LCD_SIO0 /*MOSI=D0*/, PIN_LCD_CS);
    // D1 et D2 en output simple (QSPI mode : le driver écrit en 4 bits via writeBytes)
    pinMode(PIN_LCD_SI1, OUTPUT);
    pinMode(PIN_LCD_SI2, OUTPUT);

    _spi.beginTransaction(_spi_cfg);
    _init_seq();
    _spi.endTransaction();
}

// Envoie une zone de pixels RGB565 (depuis le buffer LVGL)
void flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
           const uint16_t* color_map) {
    _spi.beginTransaction(_spi_cfg);

    // Column address set
    _cs_low();
    _spi.transfer(0x02); _spi.transfer(0x00); _spi.transfer(0x2A); _spi.transfer(0x00);
    _spi.transfer((uint8_t)(x1 >> 8)); _spi.transfer((uint8_t)x1);
    _spi.transfer((uint8_t)(x2 >> 8)); _spi.transfer((uint8_t)x2);
    _cs_high();

    // Row address set
    _cs_low();
    _spi.transfer(0x02); _spi.transfer(0x00); _spi.transfer(0x2B); _spi.transfer(0x00);
    _spi.transfer((uint8_t)(y1 >> 8)); _spi.transfer((uint8_t)y1);
    _spi.transfer((uint8_t)(y2 >> 8)); _spi.transfer((uint8_t)y2);
    _cs_high();

    // RAM Write (data QSPI)
    uint32_t npx = (uint32_t)(x2 - x1 + 1) * (uint32_t)(y2 - y1 + 1);
    _dat((const uint8_t*)color_map, npx * 2);

    _spi.endTransaction();
}

} // namespace co5300
