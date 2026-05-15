#pragma once
// ============================================================
// CompagnonV2 — hal/rtc.h
// RTC PCF85063 I2C
// Fallback NTP si WiFi disponible
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include <time.h>

#define PCF85063_ADDR     0x51
#define PCF85063_REG_CTRL 0x00
#define PCF85063_REG_SEC  0x04   // Secondes BCD

bool  rtc_init();
void  rtc_set_from_ntp();       // Sync RTC depuis NTP (appeler quand WiFi connecté)
time_t rtc_get_time();          // Timestamp UNIX
void  rtc_get_local_fr(char* buf, size_t buf_len);  // "15 mai 2026 - 16:05"
