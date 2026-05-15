// ============================================================
// CompagnonV2 — hal/touch.cpp
// CST816S I2C capacitif — Waveshare AMOLED 2.16"
// ============================================================
#include "touch.h"

static lv_indev_drv_t s_indev_drv;

// Lecture registres CST816S
static bool _cst816s_read(uint16_t* x, uint16_t* y) {
    Wire.beginTransmission(CST816S_ADDR);
    Wire.write(CST816S_REG_NPTS);
    if (Wire.endTransmission(false) != 0) return false;
    Wire.requestFrom(CST816S_ADDR, (uint8_t)6);
    if (Wire.available() < 6) return false;

    Wire.read();                           // NPTS
    Wire.read();                           // GEST
    uint8_t xh = Wire.read();
    uint8_t xl = Wire.read();
    uint8_t yh = Wire.read();
    uint8_t yl = Wire.read();

    *x = ((xh & 0x0F) << 8) | xl;
    *y = ((yh & 0x0F) << 8) | yl;
    return true;
}

void touch_read_cb(lv_indev_drv_t* drv, lv_indev_data_t* data) {
    uint16_t tx, ty;
    if (_cst816s_read(&tx, &ty)) {
        // Clamp dans la safe area
        tx = constrain((int)tx, UI_X, UI_X + UI_W - 1);
        ty = constrain((int)ty, UI_Y, UI_Y + UI_H - 1);
        data->point.x = (lv_coord_t)tx;
        data->point.y = (lv_coord_t)ty;
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

bool touch_init() {
    // Reset partagé avec l'écran — déjà géré dans display_init()
    // On initialise juste l'I2C et on vérifie la présence
    Wire.begin(IIC_SDA, IIC_SCL);

    // Test ping CST816S
    Wire.beginTransmission(CST816S_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("[TOUCH] ERREUR: CST816S non détecté sur I2C");
        return false;
    }

    lv_indev_drv_init(&s_indev_drv);
    s_indev_drv.type    = LV_INDEV_TYPE_POINTER;
    s_indev_drv.read_cb = touch_read_cb;
    lv_indev_drv_register(&s_indev_drv);

    Serial.println("[TOUCH] CST816S init OK");
    return true;
}
