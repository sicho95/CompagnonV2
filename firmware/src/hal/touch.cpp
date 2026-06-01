// ============================================================
// CompagnonV2 — hal/touch.cpp
// CST9220 via SensorLib 0.4.1 — namespace hal
// getPoint() est déprécié mais c'est la seule API disponible
// en 0.4.1 (getTouchPoints attend des int16_t[], pas de struct).
// On supprime le warning pour éviter l'erreur -Wdeprecated.
// ============================================================
#include "touch.h"
#include "../../include/pins.h"
#include <Arduino.h>
#include <Wire.h>
#include <SensorLib.h>
#include <TouchDrv.hpp>

static TouchDrvCST92xx _drv;
static bool _ok = false;

bool hal::touch_init() {
    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
    if (_drv.begin(Wire, CST92XX_SLAVE_ADDRESS, PIN_TP_RST, PIN_TP_INT)) {
        _drv.setSwapXY(false);
        _drv.setMirrorXY(false, false);
        _ok = true;
        Serial.printf("[TOUCH] CST9220 OK \u2014 %s (RST=%d INT=%d)\n",
                      _drv.getModelName(), PIN_TP_RST, PIN_TP_INT);
    } else {
        _ok = false;
        Serial.printf("[TOUCH] CST9220 init FAILED (RST=%d INT=%d)\n",
                      PIN_TP_RST, PIN_TP_INT);
    }
    return _ok;
}

bool hal::touch_read(uint16_t& x, uint16_t& y) {
    if (!_ok) return false;
    if (_drv.isPressed()) {
        int16_t tx[1] = {0};
        int16_t ty[1] = {0};
        // Suppression du warning deprecated pour getPoint en 0.4.1
        // (getTouchPoints n'existe pas encore comme méthode publique)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        uint8_t n = _drv.getPoint(tx, ty, 1);
#pragma GCC diagnostic pop
        if (n > 0) {
            int32_t lx = (int32_t)tx[0] - LCD_MARGIN_H;
            int32_t ly = (int32_t)ty[0] - LCD_MARGIN_V;
            if (lx < 0) lx = 0;
            if (ly < 0) ly = 0;
            if (lx >= LCD_WIDTH)  lx = LCD_WIDTH  - 1;
            if (ly >= LCD_HEIGHT) ly = LCD_HEIGHT - 1;
            x = (uint16_t)lx;
            y = (uint16_t)ly;
            return true;
        }
    }
    return false;
}

bool hal::touch_has_data() {
    return _ok && _drv.isPressed();
}
