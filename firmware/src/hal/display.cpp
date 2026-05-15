// ============================================================
// CompagnonV2 — hal/display.cpp
// Pilote rm67162 QSPI 4-wire — Waveshare AMOLED 2.16"
// Basé sur le pilote de Compagnon v1 (rm67162 QSPI)
// Arduino 3.3.8 + LVGL 8.4.x
// ============================================================
#include "display.h"
#include <SPI.h>

// ── Variables globales ─────────────────────────────────────────────────────
static lv_disp_draw_buf_t  s_draw_buf;
static lv_color_t*         s_buf1 = nullptr;
static lv_color_t*         s_buf2 = nullptr;
static lv_disp_drv_t       s_disp_drv;
static bool                s_display_ready = false;

// ── Commandes rm67162 ──────────────────────────────────────────────────────
#define RM67162_SLEEP_IN   0x10
#define RM67162_SLEEP_OUT  0x11
#define RM67162_DISPLAY_ON 0x29
#define RM67162_CASET      0x2A
#define RM67162_PASET      0x2B
#define RM67162_RAMWR      0x2C
#define RM67162_WBRIGHT    0x51

// ── SPI QSPI helper ──────────────────────────────────────────────────────
static void _spi_write_cmd(uint8_t cmd) {
    digitalWrite(LCD_CS, LOW);
    // Mode SPI standard pour la commande (1 bit)
    SPI.beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
    SPI.transfer(0x02);      // write command prefix rm67162
    SPI.transfer(cmd);
    SPI.transfer(0x00);
    SPI.endTransaction();
    digitalWrite(LCD_CS, HIGH);
}

static void _spi_write_data(const uint8_t* data, size_t len) {
    digitalWrite(LCD_CS, LOW);
    SPI.beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
    SPI.transfer(0x32);      // write data prefix rm67162
    SPI.transfer(0x00);
    for (size_t i = 0; i < len; i++) SPI.transfer(data[i]);
    SPI.endTransaction();
    digitalWrite(LCD_CS, HIGH);
}

static void _write_reg(uint8_t cmd, uint8_t val) {
    uint8_t buf[1] = { val };
    _spi_write_cmd(cmd);
    _spi_write_data(buf, 1);
}

// ── Initialisation du panel rm67162 ───────────────────────────────────────
static void _rm67162_init_sequence() {
    // Reset hardware
    digitalWrite(LCD_RESET, LOW);
    delay(20);
    digitalWrite(LCD_RESET, HIGH);
    delay(120);

    _spi_write_cmd(RM67162_SLEEP_OUT);
    delay(120);

    // DSI command set typique rm67162 AMOLED
    _write_reg(0xFE, 0x04);
    _write_reg(0x4E, 0x02);
    _write_reg(0xFE, 0x05);
    _write_reg(0xFB, 0x01);
    _write_reg(0xFE, 0x00);

    // Interface pixel format : 16bpp RGB565
    _write_reg(0x3A, 0x75);

    // Memory access control (landscape, RGB)
    _write_reg(0x36, 0x00);

    // Luminosité par défaut 50%
    _write_reg(RM67162_WBRIGHT, 0x80);

    delay(50);
    _spi_write_cmd(RM67162_DISPLAY_ON);
    delay(50);
}

// ── Callback LVGL flush ────────────────────────────────────────────────────
void display_flush_cb(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {
    // Définit la fenêtre de dessin
    uint16_t x1 = area->x1, x2 = area->x2;
    uint16_t y1 = area->y1, y2 = area->y2;

    uint8_t caset[4] = { (uint8_t)(x1 >> 8), (uint8_t)(x1), (uint8_t)(x2 >> 8), (uint8_t)(x2) };
    uint8_t paset[4] = { (uint8_t)(y1 >> 8), (uint8_t)(y1), (uint8_t)(y2 >> 8), (uint8_t)(y2) };

    _spi_write_cmd(RM67162_CASET);
    _spi_write_data(caset, 4);
    _spi_write_cmd(RM67162_PASET);
    _spi_write_data(paset, 4);
    _spi_write_cmd(RM67162_RAMWR);

    // Envoi des pixels (RGB565 big-endian)
    uint32_t pixel_count = (x2 - x1 + 1) * (y2 - y1 + 1);
    digitalWrite(LCD_CS, LOW);
    SPI.beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
    SPI.transfer(0x32);
    SPI.transfer(0x00);
    SPI.writeBytes((uint8_t*)color_p, pixel_count * 2);
    SPI.endTransaction();
    digitalWrite(LCD_CS, HIGH);

    lv_disp_flush_ready(drv);
}

// ── Init publique ──────────────────────────────────────────────────────────
bool display_init() {
    // SPI QSPI sur les pins définies dans pins.h
    SPI.begin(LCD_SCLK, -1, LCD_SDIO0, LCD_CS);
    pinMode(LCD_CS,    OUTPUT);
    pinMode(LCD_RESET, OUTPUT);
    digitalWrite(LCD_CS, HIGH);

    _rm67162_init_sequence();

    // Alloue les deux buffers DMA en PSRAM
    s_buf1 = (lv_color_t*) ps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t));
    s_buf2 = (lv_color_t*) ps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t));
    if (!s_buf1 || !s_buf2) {
        Serial.println("[DISPLAY] ERREUR: impossible d'allouer les buffers LVGL en PSRAM");
        return false;
    }

    lv_disp_draw_buf_init(&s_draw_buf, s_buf1, s_buf2, DISP_BUF_SIZE);

    lv_disp_drv_init(&s_disp_drv);
    s_disp_drv.hor_res  = SCREEN_W;
    s_disp_drv.ver_res  = SCREEN_H;
    s_disp_drv.flush_cb = display_flush_cb;
    s_disp_drv.draw_buf = &s_draw_buf;
    lv_disp_drv_register(&s_disp_drv);

    s_display_ready = true;
    Serial.printf("[DISPLAY] rm67162 init OK — %dx%d, bufs PSRAM OK\n", SCREEN_W, SCREEN_H);
    return true;
}

void display_set_brightness(uint16_t brightness) {
    _write_reg(RM67162_WBRIGHT, (uint8_t)constrain(brightness, 0, 255));
}

uint16_t display_get_brightness() { return 128; }  // TODO: lire registre

void display_sleep() {
    _spi_write_cmd(RM67162_SLEEP_IN);
    delay(5);
}

void display_wake() {
    _spi_write_cmd(RM67162_SLEEP_OUT);
    delay(120);
    _spi_write_cmd(RM67162_DISPLAY_ON);
}
