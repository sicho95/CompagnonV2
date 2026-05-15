#include "app_rappels.h"
#include <Arduino.h>
bool   app_rappels_start()  { Serial.println("[APP] Rappels start"); return true; }
void   app_rappels_stop()   { Serial.println("[APP] Rappels stop"); }
void   app_rappels_intent(const char* i, const char* p) {
    Serial.printf("[APP] Rappels intent=%s param=%s\n", i, p);
}
time_t app_rappels_next_epoch() { return 0; } // TODO: lire reminders.json
