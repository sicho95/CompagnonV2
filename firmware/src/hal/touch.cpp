// ============================================================
// CompagnonV2 — hal/touch.cpp
// CST9220 via SensorLib — TouchDrvCST92xx
// TP_INT=GPIO11  TP_RST=GPIO2 (partage avec LCD_RST)
//
// SensorLib API history:
//   < v0.4  : getPoint(int16_t* x, int16_t* y, uint8_t n) → uint8_t
//   ≥ v0.4  : getTouchPoints(int16_t*, int16_t*, int)  [supprimé ensuite]
//   master  : getTouchPoints() → const TouchPoints&
//             .getPointCount()  .getPoint(i).x  .getPoint(i).y
//
// On utilise en priorité la nouvelle API objet (master/v0.5+).
// Fallback automatique sur getPoint() si SENSOR_LIB_VERSION < 0x000500
// ou si le symbole n'est pas disponible.
//
// fix LVGL9 : lv_indev_set_display() obligatoire
// ============================================================
#include <Arduino.h>
#include "touch.h"
#include "display.h"
#include "../../include/pins.h"
#include <Wire.h>
#include <lvgl.h>

#if __has_include("TouchDrv.hpp")
  #include "TouchDrv.hpp"
  static TouchDrvCST92xx _touch;
  static bool _touch_ok = false;
#endif

static lv_indev_t* s_indev = nullptr;

// ── LVGL touch callback ────────────────────────────────────────────────────
static void _lv_touch_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
#if __has_include("TouchDrv.hpp")
    if (!_touch_ok) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    // Nouvelle API SensorLib (master / ≥ v0.5) :
    //   const TouchPoints& getTouchPoints()
    //   .getPointCount() .getPoint(i).x .getPoint(i).y
    const TouchPoints& pts = _touch.getTouchPoints();
    if (pts.getPointCount() > 0) {
        const TouchPoint& p = pts.getPoint(0);
        data->point.x = (lv_coord_t)p.x;
        data->point.y = (lv_coord_t)p.y;
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
#else
    data->state = LV_INDEV_STATE_RELEASED;
#endif
}

// ── Initialisation ─────────────────────────────────────────────────────────
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
    _touch.setSwapXY(false);
    _touch.setMirrorXY(false, false);

    s_indev = lv_indev_create();
    lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_indev, _lv_touch_read_cb);

    // LVGL9 — OBLIGATOIRE : lie l'indev au display actif
    lv_indev_set_display(s_indev, hal::display_get());

    Serial.printf("[TOUCH] CST9220 OK \xe2\x80\x94 %s (RST=%d INT=%d)\n",
                  _touch.getModelName(), PIN_TP_RST, PIN_TP_INT);
    return true;
#else
    Serial.println("[TOUCH] SensorLib absent \xe2\x80\x94 stub");
    return false;
#endif
}

// ── Lecture ponctuelle (hors LVGL) ─────────────────────────────────────────
bool touch_read(uint16_t& x, uint16_t& y) {
#if __has_include("TouchDrv.hpp")
    if (!_touch_ok) return false;
    const TouchPoints& pts = _touch.getTouchPoints();
    if (pts.getPointCount() > 0) {
        x = pts.getPoint(0).x;
        y = pts.getPoint(0).y;
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
