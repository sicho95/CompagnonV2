// ============================================================
// main.cpp — Entry point CompagnonV2 (Arduino)
// setup() bootstrap → os_start() lance les tâches FreeRTOS
// loop() vide — FreeRTOS prend la main
// ============================================================
#include <Arduino.h>
#include "system/os_main.h"

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== CompagnonV2 boot ===");
    os::os_start();
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
