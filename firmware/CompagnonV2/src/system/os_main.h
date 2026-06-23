#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace os {

extern SemaphoreHandle_t g_lvgl_mutex;

void os_start();

} // namespace os
