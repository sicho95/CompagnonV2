// =============================================================
// CompagnonV2 — hal/touch.cpp
// Driver CST9220 — API LVGL 9
// =============================================================
#include "touch.h"

SensorCST9220 touch;

bool touch_init() {
    // SensorLib initialise le bus I2C en interne si besoin
    if (!touch.begin(Wire, CST9220_SLAVE_ADDRESS, PIN_IIC_SDA, PIN_IIC_SCL)) {
        Serial.println("[HAL] Touch CST9220 NOT found!");
        return false;
    }
    Serial.println("[HAL] Touch CST9220 init OK");
    return true;
}

// Callback LVGL 9 : signature lv_indev_read_cb_t
// (lv_indev_t* remplace lv_indev_drv_t* de LVGL 8)
void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    if (touch.isAvailable()) {
        data->point.x = (lv_coord_t)touch.getX();
        data->point.y = (lv_coord_t)touch.getY();
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
