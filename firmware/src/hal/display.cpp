// ============================================================
// CompagnonV2 — HAL Display — RM67162 QSPI
// Waveshare AMOLED 2.16" 480×480
//
// Basé sur le driver officiel Waveshare Arduino + portage
// Compagnon v1 (compagnon/src/hal/)
//
// QSPI : 4 lignes de données + 1 horloge + 1 CS
// Refresh LVGL : double buffer PSRAM + DMA SPI
// ============================================================
#include "display.h"
#include <SPI.h>

// ─── Variables internes ───────────────────────────────────────────────────────
static lv_display_t* _lv_disp    = nullptr;
static lv_color_t*   _buf1       = nullptr;
static lv_color_t*   _buf2       = nullptr;
static bool          _display_on = false;
static uint8_t       _brightness = 200;

// ─── Helpers bas niveau RM67162 ──────────────────────────────────────────────

static void rm67162_send_cmd(uint32_t cmd, const uint8_t* data, size_t len) {
    // Mode CMD : SPI avec SCLK + SDIO0 (1-bit) ou QSPI (4-bit) selon phase
    // Le RM67162 utilise la commande sur 1 octet en mode SPI standard
    // puis les données en QSPI 4-bit pour les transferts pixel
    digitalWrite(LCD_CS, LOW);
    // DC=0 pour commande : on utilise SDIO0 en mode 1-bit
    // Arduino SPI standard pour commandes (compatibilité maximale)
    SPI.beginTransaction(SPISettings(80000000, MSBFIRST, SPI_MODE0));
    SPI.transfer((uint8_t)(cmd & 0xFF));
    if (data && len > 0) {
        SPI.writeBytes(data, len);
    }
    SPI.endTransaction();
    digitalWrite(LCD_CS, HIGH);
}

static void rm67162_reset(void) {
    // LCD_RESET est partagé avec TOUCH_RES (cf. pins.h)
    digitalWrite(LCD_RESET, LOW);
    delay(10);
    digitalWrite(LCD_RESET, HIGH);
    delay(120);
}

static void rm67162_init_sequence(void) {
    // Séquence d'initialisation RM67162 pour Waveshare AMOLED 2.16"
    // Source : datasheet RM67162 + Waveshare sample code
    const uint8_t d0[]  = {0x00};
    const uint8_t d1[]  = {0x5A};
    const uint8_t d2[]  = {0x3B};
    const uint8_t d3[]  = {0x00, 0x00, 0x01, 0xDF}; // CASET 0..479
    const uint8_t d4[]  = {0x00, 0x00, 0x01, 0xDF}; // RASET 0..479
    const uint8_t d5[]  = {0x55};  // RGB565
    const uint8_t d6[]  = {0x20};  // TEARING OFF
    const uint8_t d7[]  = {0x28};  // Backlight control

    rm67162_send_cmd(0xFE, d1, 1);   // Enter CMD2
    rm67162_send_cmd(0xC4, d0, 1);
    rm67162_send_cmd(0xFE, d0, 1);   // Back to CMD1
    rm67162_send_cmd(0x35, nullptr, 0); // TEARING ON
    rm67162_send_cmd(0x3A, d5, 1);   // Interface Pixel Format RGB565
    rm67162_send_cmd(0x2A, d3, 4);   // Column addr 0..479
    rm67162_send_cmd(0x2B, d4, 4);   // Row addr 0..479
    rm67162_send_cmd(0x11, nullptr, 0); // Sleep Out
    delay(120);
    rm67162_send_cmd(0x29, nullptr, 0); // Display ON
    delay(20);

    _display_on = true;
}

// ─── LVGL flush callback ──────────────────────────────────────────────────────

static void lvgl_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    // Calcul de la fenêtre de pixels
    uint16_t x1 = area->x1;
    uint16_t x2 = area->x2;
    uint16_t y1 = area->y1;
    uint16_t y2 = area->y2;
    uint32_t len = (x2 - x1 + 1) * (y2 - y1 + 1) * 2; // RGB565 = 2 octets/pixel

    // CASET + RASET
    uint8_t caset[4] = {(uint8_t)(x1 >> 8), (uint8_t)(x1), (uint8_t)(x2 >> 8), (uint8_t)(x2)};
    uint8_t raset[4] = {(uint8_t)(y1 >> 8), (uint8_t)(y1), (uint8_t)(y2 >> 8), (uint8_t)(y2)};
    rm67162_send_cmd(0x2A, caset, 4);
    rm67162_send_cmd(0x2B, raset, 4);

    // RAMWR + transfert pixels
    digitalWrite(LCD_CS, LOW);
    SPI.beginTransaction(SPISettings(80000000, MSBFIRST, SPI_MODE0));
    SPI.transfer(0x2C); // RAMWR cmd
    SPI.writeBytes(px_map, len);
    SPI.endTransaction();
    digitalWrite(LCD_CS, HIGH);

    lv_display_flush_ready(disp);
}

// ─── API publique ─────────────────────────────────────────────────────────────

void hal_display_init(void) {
    // Config GPIO
    pinMode(LCD_CS,    OUTPUT);
    pinMode(LCD_SCLK,  OUTPUT);
    pinMode(LCD_RESET, OUTPUT);
    digitalWrite(LCD_CS, HIGH);

    // SPI bus — QSPI full n'est pas dispo via Arduino SPI standard
    // On utilise SPI.begin avec les pins QSPI0/SCLK/CS pour la compat
    // Le QSPI 4-bit réel nécessitera esp_lcd_panel_io (possible en Phase 2)
    SPI.begin(LCD_SCLK, LCD_SDIO1, LCD_SDIO0, LCD_CS);

    // Reset hardware
    rm67162_reset();

    // Séquence init RM67162
    rm67162_init_sequence();

    // Allocate LVGL draw buffers in PSRAM
    _buf1 = (lv_color_t*) ps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t));
    _buf2 = (lv_color_t*) ps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t));
    assert(_buf1 != nullptr && _buf2 != nullptr);

    // Enregistrement display LVGL 9
    _lv_disp = lv_display_create(SCREEN_W, SCREEN_H);
    lv_display_set_flush_cb(_lv_disp, lvgl_flush_cb);
    lv_display_set_buffers(_lv_disp, _buf1, _buf2,
                           DISP_BUF_SIZE * sizeof(lv_color_t),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    Serial.printf("[HAL] Display RM67162 init OK — %dx%d\n", SCREEN_W, SCREEN_H);
}

void hal_display_on(void) {
    rm67162_send_cmd(0x29, nullptr, 0); // Display ON
    _display_on = true;
}

void hal_display_off(void) {
    rm67162_send_cmd(0x28, nullptr, 0); // Display OFF
    _display_on = false;
}

void hal_display_set_brightness(uint8_t brightness) {
    _brightness = brightness;
    uint8_t d[] = {brightness};
    rm67162_send_cmd(0x51, d, 1); // Write display brightness
}

bool hal_display_is_on(void) {
    return _display_on;
}
