#include "display.h"
#include <lvgl.h>
#include <Arduino.h>
// display.cpp — stub minimal
// L'implémentation réelle est dans hal/drivers/co5300.cpp
// Ces fonctions sont les wrappers haut niveau.

static lv_display_t* _disp = nullptr;

namespace hal {

void display_init() {
    // Initialisation pilotée par co5300_init() appelé depuis main.cpp
    Serial.println("[HAL] display_init (stub)");
}

lv_display_t* display_get() {
    return _disp;
}

void display_set_brightness(uint8_t pct) {
    // TODO: PWM backlight si disponible
    (void)pct;
}

void display_sleep() {
    if (_disp) lv_display_set_rotation(_disp, LV_DISPLAY_ROTATION_0);
    Serial.println("[HAL] display_sleep");
}

void display_wakeup() {
    Serial.println("[HAL] display_wakeup");
}

void display_force_refresh() {
    lv_refr_now(nullptr);
}

} // namespace hal
