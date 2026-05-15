// =============================================================
// CompagnonV2 — hal/touch.cpp
// Driver CST9220 — LVGL 9 API
// =============================================================
#include "touch.h"

SensorCST9220 touch;

bool touch_init() {
    if (!touch.begin(Wire, CST9220_SLAVE_ADDRESS, PIN_IIC_SDA, PIN_IIC_SCL)) {
        Serial.println("[HAL] Touch CST9220 NOT found!");
        return false;
    }
    Serial.println("[HAL] Touch CST9220 init OK");
    return true;
}

// LVGL 9 : signature (lv_indev_t*, lv_indev_data_t*)
void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    if (touch.isAvailable()) {
        data->point.x = touch.getX();
        data->point.y = touch.getY();
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
