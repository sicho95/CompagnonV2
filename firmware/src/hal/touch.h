// ============================================================
// CompagnonV2 — touch.h
// CST9220 — TP_INT=GPIO11, TP_RST=GPIO40
// ============================================================
#pragma once
#include <stdint.h>

namespace hal {

bool touch_init();
bool touch_read(uint16_t &x, uint16_t &y);
bool touch_has_data();

} // namespace hal
