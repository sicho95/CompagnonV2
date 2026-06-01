#pragma once
// ============================================================
// CompagnonV2 — ui/ui_dispatch.h
// Queue thread-safe pour appels LVGL hors task_ui_lvgl.
// dispatch_post_sync() bloque l'appelant jusqu'à exécution.
// ============================================================
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <functional>

namespace ui {

using UiTask = std::function<void()>;

void dispatch_init();
bool dispatch_post(UiTask fn);       // fire-and-forget
void dispatch_post_sync(UiTask fn);  // bloque jusqu'à exécution (max 2s)
void dispatch_flush();               // appelé dans task_ui_lvgl

} // namespace ui
