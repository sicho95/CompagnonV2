#pragma once
// ============================================================
// CompagnonV2 — hal/pmu.h
// Gestion PMU AXP2101 via XPowersLib
// Waveshare ESP32-S3 AMOLED 2.16"
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include <XPowersLib.h>
#include "../config/pins.h"

// Données batterie partagées avec le reste de l'OS
struct BatteryInfo {
    uint8_t  percent;    // 0-100 %
    float    voltage;    // mV
    bool     charging;
    bool     usb_present;
};

bool        pmu_init();
void        pmu_tick();          // à appeler dans task_os_main ~1s
BatteryInfo pmu_get_battery();   // thread-safe (copie)
void        pmu_set_charging_led(bool on);
