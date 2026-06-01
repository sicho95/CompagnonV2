// ============================================================
// CompagnonV2 — ui/ui_dispatch.cpp
// ============================================================
#include "ui_dispatch.h"
#include <Arduino.h>

#define UI_QUEUE_LEN 8

namespace ui {

struct UiMsg { UiTask fn; };

static QueueHandle_t _q = nullptr;

void dispatch_init() {
    _q = xQueueCreate(UI_QUEUE_LEN, sizeof(UiMsg));
}

bool dispatch_post(UiTask fn) {
    if (!_q) return false;
    UiMsg msg { fn };
    return xQueueSend(_q, &msg, 0) == pdTRUE;
}

void dispatch_flush() {
    if (!_q) return;
    UiMsg msg;
    while (xQueueReceive(_q, &msg, 0) == pdTRUE) {
        if (msg.fn) msg.fn();
    }
}

} // namespace ui
