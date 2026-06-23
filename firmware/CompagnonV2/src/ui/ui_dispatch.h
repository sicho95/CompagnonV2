#pragma once
// ============================================================
// CompagnonV2 — ui/ui_dispatch.h
// Queue thread-safe pour appels LVGL hors task_ui_lvgl.
// dispatch_post_sync() SUPPRIME — causait deadlock WDT Core 1.
// Toutes les lambdas sont fire-and-forget.
// ============================================================
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <functional>

namespace ui {

using UiTask = std::function<void()>;

void dispatch_init();
bool dispatch_post(UiTask fn);  // fire-and-forget, thread-safe
uint8_t dispatch_flush();       // appelé UNIQUEMENT dans task_ui_lvgl
bool dispatch_is_ui_thread();

} // namespace ui
