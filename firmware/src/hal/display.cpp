// =============================================================
// CompagnonV2 — hal/display.cpp
// =============================================================
#include "display.h"

// ── Bus QSPI ──────────────────────────────────────────────────
static Arduino_DataBus *_bus = new Arduino_ESP32QSPI(
    PIN_LCD_CS,    // CS
    PIN_LCD_SCLK,  // SCK
    PIN_LCD_SDIO0, // SDIO0
    PIN_LCD_SDIO1, // SDIO1
    PIN_LCD_SDIO2, // SDIO2
    PIN_LCD_SDIO3  // SDIO3
);

// ── Display CO5300 ────────────────────────────────────────────
Arduino_CO5300 *gfx = new Arduino_CO5300(
    _bus,
    PIN_LCD_RESET, // RST
    0,             // rotation 0
    LCD_WIDTH,
    LCD_HEIGHT,
    0, 0, 0, 0     // offsets
);

void display_init() {
    gfx->begin();
    // Registre d'orientation requis par le CO5300 sur cette carte
    _bus->writeC8D8(0x36, 0xA0);
    Serial.println("[HAL] Display CO5300 QSPI init OK");
}

void display_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif
    lv_disp_flush_ready(disp);
}

void display_rounder_cb(struct _lv_disp_drv_t *disp_drv, lv_area_t *area) {
    // Le CO5300 exige des coordonnées paires
    if (area->x1 % 2 != 0) area->x1--;
    if (area->y1 % 2 != 0) area->y1--;
    if (area->x2 % 2 == 0) area->x2++;
    if (area->y2 % 2 == 0) area->y2++;
}
