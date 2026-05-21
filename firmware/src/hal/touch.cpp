// ============================================================
// CompagnonV2 — hal/touch.cpp
// CST9220 via SensorLib TouchDrv.hpp (API unifiée v0.4+)
// TP_INT=GPIO11  TP_RST=GPIO2 (partagé avec LCD_RST)
// Adresse SensorLib : 0x5A
// Référence : Waveshare examples/Arduino-v3.3.5/05_LVGL_Widgets
// ============================================================
#include <Arduino.h>
#include "touch.h"
#include "../../include/pins.h"
#include <Wire.h>

// TouchDrv.hpp = header unifié SensorLib 0.4+ (remplace TouchDrvCSTXXX.hpp deprecated)
#if __has_include("TouchDrv.hpp")
  #include "TouchDrv.hpp"
  static TouchDrvCST92xx _touch;
  static bool _touch_ok = false;
#endif

namespace hal {

bool touch_init() {
#if __has_include("TouchDrv.hpp")
    // Reset séquence (GPIO2 partagé avec LCD_RST — déjà HIGH à ce stade)
    pinMode(PIN_TP_RST, OUTPUT);
    digitalWrite(PIN_TP_RST, LOW);  delay(30);
    digitalWrite(PIN_TP_RST, HIGH); delay(50);

    _touch.setPins(PIN_TP_RST, PIN_TP_INT);
    _touch_ok = _touch.begin(Wire, 0x5A, PIN_IIC_SDA, PIN_IIC_SCL);

    if (!_touch_ok) {
        Serial.printf("[TOUCH] CST9220 not found (SensorLib 0x5A RST=%d INT=%d)\n",
                      PIN_TP_RST, PIN_TP_INT);
        return false;
    }

    _touch.setMaxCoordinates(LCD_WIDTH, LCD_HEIGHT);
    _touch.setSwapXY(true);
    _touch.setMirrorXY(true, false);

    Serial.printf("[TOUCH] CST9220 OK — %s (RST=%d INT=%d)\n",
                  _touch.getModelName(), PIN_TP_RST, PIN_TP_INT);
    return true;
#else
    Serial.println("[TOUCH] SensorLib absent — stub");
    return false;
#endif
}

bool touch_read(uint16_t &x, uint16_t &y) {
#if __has_include("TouchDrv.hpp")
    if (!_touch_ok) return false;
    int16_t tx[1], ty[1];
    // getTouchPoints() = API v0.4+ (getPoint() deprecated)
    if (_touch.getTouchPoints(tx, ty, 1) == 0) return false;
    x = (uint16_t)tx[0];
    y = (uint16_t)ty[0];
    return true;
#else
    return false;
#endif
}

bool touch_has_data() {
    return digitalRead(PIN_TP_INT) == LOW;
}

} // namespace hal
