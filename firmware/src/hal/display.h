#pragma once
#include <lvgl.h>
#include <Arduino_GFX_Library.h>

#ifdef __cplusplus
extern "C" {
#endif

void          hal_display_init();
lv_display_t* hal_display_get();
void          hal_display_set_brightness(uint8_t pct);
void          hal_display_sleep();
void          hal_display_wakeup();

#ifdef __cplusplus
}
#endif
