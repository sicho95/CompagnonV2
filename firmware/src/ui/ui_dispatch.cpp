// ============================================================
// CompagnonV2 — ui/ui_dispatch.cpp
// ============================================================
#include "ui_dispatch.h"
#include <Arduino.h>

#define UI_QUEUE_LEN 8

namespace ui {

struct UiMsg {
    UiTask           fn;
    SemaphoreHandle_t done; // nullptr = fire-and-forget
};

static QueueHandle_t _q = nullptr;

void dispatch_init() {
    _q = xQueueCreate(UI_QUEUE_LEN, sizeof(UiMsg));
}

bool dispatch_post(UiTask fn) {
    if (!_q) return false;
    UiMsg msg { fn, nullptr };
    return xQueueSend(_q, &msg, 0) == pdTRUE;
}

void dispatch_post_sync(UiTask fn) {
    if (!_q) { fn(); return; }
    SemaphoreHandle_t sem = xSemaphoreCreateBinary();
    UiMsg msg { fn, sem };
    if (xQueueSend(_q, &msg, pdMS_TO_TICKS(500)) == pdTRUE) {
        // attend que dispatch_flush() ait exécuté la tâche (max 2s)
        xSemaphoreTake(sem, pdMS_TO_TICKS(2000));
    } else {
        Serial.println("[DISPATCH] queue full — sync fallback");
        fn(); // fallback direct (risque, mais vaut mieux qu'un freeze)
    }
    vSemaphoreDelete(sem);
}

void dispatch_flush() {
    if (!_q) return;
    UiMsg msg;
    while (xQueueReceive(_q, &msg, 0) == pdTRUE) {
        if (msg.fn) msg.fn();
        if (msg.done) xSemaphoreGive(msg.done);
    }
}

} // namespace ui
