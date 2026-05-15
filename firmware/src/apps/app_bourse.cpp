#include "app_bourse.h"
#include <Arduino.h>
bool app_bourse_start() { Serial.println("[APP] Bourse start"); return true; }
void app_bourse_stop()  { Serial.println("[APP] Bourse stop"); }
