// ============================================================
// CompagnonV2 — HAL Touch — CST816S I2C
// ============================================================
#include "touch.h"
#include <lvgl.h>

// Registres CST816S
#define REG_GESTURE    0x01
#define REG_FINGER_NUM 0x02
#define REG_XH         0x03
#define REG_XL         0x04
#define REG_YH         0x05
#define REG_YL         0x06

static lv_indev_t* _touch_indev = nullptr;
static touch_data_t _last_touch = {false, 0, 0, 0};

static bool cst816s_read_raw(touch_data_t* out) {
    Wire.beginTransmission(CST816S_ADDR);
    Wire.write(REG_GESTURE);
    if (Wire.endTransmission(false) != 0) {
        out->pressed = false;
        return false;
    }
    Wire.requestFrom((uint8_t)CST816S_ADDR, (uint8_t)6);
    if (Wire.available() < 6) {
        out->pressed = false;
        return false;
    }
    out->gesture    = Wire.read();           // 0x01
    uint8_t fingers = Wire.read();           // 0x02
    uint8_t xh      = Wire.read();           // 0x03
    uint8_t xl      = Wire.read();           // 0x04
    uint8_t yh      = Wire.read();           // 0x05
    uint8_t yl      = Wire.read();           // 0x06

    out->pressed = (fingers > 0);
    out->x = ((int16_t)(xh & 0x0F) << 8) | xl;
    out->y = ((int16_t)(yh & 0x0F) << 8) | yl;
    return true;
}

void hal_touch_lvgl_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    touch_data_t td;
    if (cst816s_read_raw(&td) && td.pressed) {
        data->point.x  = td.x;
        data->point.y  = td.y;
        data->state    = LV_INDEV_STATE_PRESSED;
        _last_touch    = td;
    } else {
        data->point.x  = _last_touch.x;
        data->point.y  = _last_touch.y;
        data->state    = LV_INDEV_STATE_RELEASED;
    }
}

void hal_touch_init(void) {
    // Reset du touch (même pin que LCD_RESET)
    // Le reset a déjà été fait par hal_display_init, on attend juste
    delay(10);

    // Vérification présence CST816S
    Wire.beginTransmission(CST816S_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("[HAL] Touch CST816S NOT found on I2C!");
        return;
    }

    // Enregistrement indev LVGL 9
    _touch_indev = lv_indev_create();
    lv_indev_set_type(_touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(_touch_indev, hal_touch_lvgl_read_cb);

    Serial.println("[HAL] Touch CST816S init OK");
}

bool hal_touch_read(touch_data_t* out) {
    return cst816s_read_raw(out);
}
