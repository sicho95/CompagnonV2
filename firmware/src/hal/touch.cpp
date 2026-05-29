// ============================================================
// CompagnonV2 — hal/touch.cpp
// CST9220 via SensorLib TouchDrv.hpp (API unifiee v0.4+)
// TP_INT=GPIO11  TP_RST=GPIO2 (partage avec LCD_RST)
// fix: getPoint() direct + swap/mirror corriges pour ROTATION_0
// ============================================================
#include <Arduino.h>
#include "touch.h"
#include "../../include/pins.h"
#include <Wire.h>
#include <lvgl.h>

#if __has_include("TouchDrv.hpp")
  #include "TouchDrv.hpp"
  static TouchDrvCST92xx _touch;
  static bool _touch_ok = false;
#endif

static lv_indev_t* s_indev = nullptr;

static void _lv_touch_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
#if __has_include("TouchDrv.hpp")
    if (!_touch_ok) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    int16_t x = 0, y = 0;
    uint8_t touched = _touch.getPoint(&x, &y, 1);
    if (touched > 0) {
        data->point.x = (lv_coord_t)x;
        data->point.y = (lv_coord_t)y;
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
#else
    data->state = LV_INDEV_STATE_RELEASED;
#endif
}

namespace hal {

bool touch_init() {
#if __has_include("TouchDrv.hpp")
    pinMode(PIN_TP_RST, OUTPUT);
    digitalWrite(PIN_TP_RST, LOW);  delay(30);
    digitalWrite(PIN_TP_RST, HIGH); delay(50);

    _touch.setPins(PIN_TP_RST, PIN_TP_INT);
    _touch_ok = _touch.begin(Wire, 0x5A, PIN_IIC_SDA, PIN_IIC_SCL);

    if (!_touch_ok) {
        Serial.printf("[TOUCH] CST9220 not found (0x5A RST=%d INT=%d)\n",
                      PIN_TP_RST, PIN_TP_INT);
        return false;
    }

    _touch.setMaxCoordinates(LCD_WIDTH, LCD_HEIGHT);
    // ROTATION_0 : pas de swap ni mirror
    _touch.setSwapXY(false);
    _touch.setMirrorXY(false, false);

    s_indev = lv_indev_create();
    lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_indev, _lv_touch_read_cb);

    Serial.printf("[TOUCH] CST9220 OK \xe2\x80\x94 %s (RST=%d INT=%d)\n",
                  _touch.getModelName(), PIN_TP_RST, PIN_TP_INT);
    return true;
#else
    Serial.println("[TOUCH] SensorLib absent \xe2\x80\x94 stub");
    return false;
#endif
}

bool touch_read(uint16_t& x, uint16_t& y) {
#if __has_include("TouchDrv.hpp")
    if (!_touch_ok) return false;
    int16_t tx = 0, ty = 0;
    if (_touch.getPoint(&tx, &ty, 1) > 0) {
        x = (uint16_t)tx;
        y = (uint16_t)ty;
        return true;
    }
    return false;
#else
    return false;
#endif
}

bool touch_has_data() {
    return digitalRead(PIN_TP_INT) == LOW;
}

} // namespace hal
