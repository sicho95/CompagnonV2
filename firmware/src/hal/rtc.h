// ============================================================
// CompagnonV2 — hal/rtc.h
// PCF85063 RTC — heure offline garantie au boot
// ============================================================
#pragma once
#include <time.h>
#include <stdbool.h>

namespace hal {

bool   rtc_init();                    // init PCF85063 via I2C
bool   rtc_is_valid();                // true si heure jamais réglée
void   rtc_apply_to_system();         // PCF85063 → settimeofday()
void   rtc_sync_from_ntp(time_t epoch); // NTP epoch → PCF85063 + system
void   rtc_sync_from_pwa(time_t epoch); // BLE SET_TIME → PCF85063 + system
time_t rtc_get_epoch();               // lit epoch UTC depuis PCF85063
struct tm rtc_get_local_time();       // epoch → localtime (TZ Europe/Paris)
void   rtc_set_alarm(time_t epoch);   // programme alarme matérielle
void   rtc_clear_alarm();

} // namespace hal
