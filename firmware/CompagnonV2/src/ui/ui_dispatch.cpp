// ============================================================
// CompagnonV2 — ui/ui_dispatch.cpp
// Fix deadlock : dispatch_post_sync() supprimé.
// Seule dispatch_post (fire-and-forget) subsiste.
// ============================================================
#include "ui_dispatch.h"
#include <Arduino.h>

#define UI_QUEUE_LEN 12

namespace ui {

static QueueHandle_t _q = nullptr;
static TaskHandle_t  _ui_task = nullptr;

void dispatch_init() {
    _q = xQueueCreate(UI_QUEUE_LEN, sizeof(UiTask));
    _ui_task = xTaskGetCurrentTaskHandle();
}

bool dispatch_post(UiTask fn) {
    if (!_q || !fn) return false;
    // On alloue la std::function sur le heap pour la passer par pointeur
    UiTask* heap_fn = new UiTask(std::move(fn));
    if (xQueueSend(_q, &heap_fn, pdMS_TO_TICKS(50)) != pdTRUE) {
        Serial.println("[DISPATCH] queue full — drop");
        delete heap_fn;
        return false;
    }
    return true;
}

uint8_t dispatch_flush() {
    if (!_q) return 0;
    uint8_t count = 0;
    UiTask* fn_ptr = nullptr;
    while (xQueueReceive(_q, &fn_ptr, 0) == pdTRUE) {
        ++count;
        if (fn_ptr) {
            (*fn_ptr)();
            delete fn_ptr;
        }
    }
    return count;
}

bool dispatch_is_ui_thread() {
    return _ui_task && xTaskGetCurrentTaskHandle() == _ui_task;
}

} // namespace ui
