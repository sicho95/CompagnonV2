// ============================================================
// CompagnonV2 — hal/rtc.cpp
// PCF85063 RTC — boot offline, sync NTP/BLE
// Stratégie heure :
//   Boot → lire PCF85063 → settimeofday (valide même sans WiFi)
//   WiFi up → NTP → réécrire PCF85063
//   BLE SET_TIME → réécrire PCF85063 (réglage manuel / premier boot)
// ============================================================
#include "rtc.h"
#include "../../include/pins.h"
#include <Wire.h>
#include <sys/time.h>

// SensorLib PCF85063 (XPowersLib ou SensorLib selon board package)
#include <SensorPCF85063.hpp>

namespace hal {

static SensorPCF85063 _rtc;
static bool _rtc_ok    = false;
static bool _rtc_valid = false; // false si jamais synchronisé (epoch < 2020)

// Epoch minimal considéré comme valide : 2020-01-01 00:00:00 UTC
#define EPOCH_MIN_VALID 1577836800UL

// fix: timegm() absent sur ESP32 — implémentation UTC manuelle
static time_t _timegm_utc(struct tm* t) {
    const char* tz = getenv("TZ");
    setenv("TZ", "UTC0", 1);
    tzset();
    time_t result = mktime(t);
    if (tz) setenv("TZ", tz, 1);
    else    unsetenv("TZ");
    tzset();
    return result;
}

bool rtc_init() {
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    if (!_rtc.begin(Wire, PIN_IIC_SDA, PIN_IIC_SCL)) {
        Serial.println("[RTC] PCF85063 NOT found on I2C");
        return false;
    }
    _rtc_ok = true;
    time_t ep = rtc_get_epoch();
    _rtc_valid = (ep > EPOCH_MIN_VALID);
    Serial.printf("[RTC] PCF85063 OK — epoch=%lu valid=%s\n",
        (unsigned long)ep, _rtc_valid ? "YES" : "NO (needs sync)");
    return true;
}

bool rtc_is_valid() {
    return _rtc_ok && _rtc_valid;
}

void rtc_apply_to_system() {
    if (!_rtc_ok) return;
    time_t ep = rtc_get_epoch();
    if (ep < EPOCH_MIN_VALID) {
        Serial.println("[RTC] Heure invalide — system time non appliqué");
        return;
    }
    struct timeval tv = { .tv_sec = ep, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    struct tm t;
    localtime_r(&ep, &t);
    Serial.printf("[RTC] System time set from RTC: %04d-%02d-%02d %02d:%02d:%02d (local)\n",
        t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
}

static void _write_rtc(time_t epoch) {
    struct tm t;
    gmtime_r(&epoch, &t);
    _rtc.setDateTime(t.tm_year+1900, t.tm_mon+1, t.tm_mday,
                     t.tm_hour, t.tm_min, t.tm_sec);
    struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    _rtc_valid = true;
}

void rtc_sync_from_ntp(time_t epoch) {
    if (!_rtc_ok || epoch < EPOCH_MIN_VALID) return;
    _write_rtc(epoch);
    struct tm t; gmtime_r(&epoch, &t);
    Serial.printf("[RTC] Synced from NTP: %04d-%02d-%02d %02d:%02d:%02d UTC\n",
        t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
}

void rtc_sync_from_pwa(time_t epoch) {
    if (!_rtc_ok || epoch < EPOCH_MIN_VALID) return;
    _write_rtc(epoch);
    Serial.println("[RTC] Synced from PWA via BLE");
}

time_t rtc_get_epoch() {
    if (!_rtc_ok) return 0;
    RTC_DateTime dt = _rtc.getDateTime();
    struct tm t = {};
    // fix: SensorLib 0.4.1 — membres privés, utiliser les getters
    t.tm_year  = dt.getYear()  - 1900;
    t.tm_mon   = dt.getMonth() - 1;
    t.tm_mday  = dt.getDay();
    t.tm_hour  = dt.getHour();
    t.tm_min   = dt.getMinute();
    t.tm_sec   = dt.getSecond();
    t.tm_isdst = 0;
    return _timegm_utc(&t);
}

struct tm rtc_get_local_time() {
    time_t ep = rtc_get_epoch();
    struct tm local = {};
    localtime_r(&ep, &local);
    return local;
}

void rtc_set_alarm(time_t epoch) {
    if (!_rtc_ok) return;
    struct tm t; gmtime_r(&epoch, &t);
    _rtc.setAlarm(t.tm_hour, t.tm_min, 0, t.tm_mday, 0xFF);
    _rtc.enableAlarm();
    Serial.printf("[RTC] Alarm set: %02d/%02d %02d:%02d UTC\n",
        t.tm_mday, t.tm_mon+1, t.tm_hour, t.tm_min);
}

void rtc_clear_alarm() {
    if (!_rtc_ok) return;
    _rtc.disableAlarm();
}

} // namespace hal
