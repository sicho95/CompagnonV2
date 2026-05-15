// ============================================================
// CompagnonV2 — apps/app_nestor.cpp
// Stub — à implémenter (Step 4)
// ============================================================
#include "app_nestor.h"
#include <Arduino.h>
bool app_nestor_start()  { Serial.println("[APP] Nestor start");  return true; }
void app_nestor_stop()   { Serial.println("[APP] Nestor stop"); }
void app_nestor_intent(const char* i, const char* p) {
    Serial.printf("[APP] Nestor intent=%s param=%s\n", i, p);
}
