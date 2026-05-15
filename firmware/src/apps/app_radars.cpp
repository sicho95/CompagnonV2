#include "app_radars.h"
#include <Arduino.h>
bool app_radars_start() { Serial.println("[APP] Radars start"); return true; }
void app_radars_stop()  { Serial.println("[APP] Radars stop"); }
