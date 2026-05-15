// =============================================================
// CompagnonV2 — hal/touch.cpp
// =============================================================
#include "touch.h"

SensorCST816S touch;

bool touch_init() {
    // RST et INT sont gérés par SensorLib
    if (!touch.begin(Wire, I2C_ADDR_CST816S, PIN_IIC_SDA, PIN_IIC_SCL)) {
        Serial.println("[HAL] Touch CST816S NOT found!");
        return false;
    }
    touch.setCSTMode(CST816S_MODE_INTERRUPT); // mode interruption
    Serial.println("[HAL] Touch CST816S init OK");
    return true;
}

void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    if (touch.isAvailable()) {
        data->point.x = touch.getX();
        data->point.y = touch.getY();
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
