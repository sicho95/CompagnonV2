#include "app_meteo.h"
#include <Arduino.h>
bool app_meteo_start() { Serial.println("[APP] Meteo start"); return true; }
void app_meteo_stop()  { Serial.println("[APP] Meteo stop"); }
