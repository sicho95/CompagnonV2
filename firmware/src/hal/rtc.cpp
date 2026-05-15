// =============================================================
// CompagnonV2 — hal/rtc.cpp
// =============================================================
#include "rtc.h"
#include <time.h>

SensorPCF85063 rtc;

bool rtc_init() {
    if (!rtc.begin(Wire, PCF85063_SLAVE_ADDRESS, PIN_IIC_SDA, PIN_IIC_SCL)) {
        Serial.println("[HAL] RTC PCF85063 NOT found!");
        return false;
    }
    // Configuration timezone pour la France (CET/CEST)
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    Serial.println("[HAL] RTC PCF85063 init OK");
    return true;
}

void rtc_sync_from_ntp(time_t epoch) {
    // Convertit epoch UTC → struct tm UTC
    struct tm t;
    gmtime_r(&epoch, &t);
    // Écrit dans la RTC
    rtc.setDateTime(
        t.tm_year + 1900,
        t.tm_mon  + 1,
        t.tm_mday,
        t.tm_hour,
        t.tm_min,
        t.tm_sec
    );
    // Met aussi à jour l'horloge système ESP32
    struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    Serial.printf("[HAL] RTC synced from NTP : %04d-%02d-%02d %02d:%02d:%02d UTC\n",
        t.tm_year+1900, t.tm_mon+1, t.tm_mday,
        t.tm_hour, t.tm_min, t.tm_sec);
}

struct tm rtc_get_local_time() {
    // Lit depuis la RTC hardware (UTC) puis convertit via TZ
    RTC_DateTime dt = rtc.getDateTime();
    struct tm t = {};
    t.tm_year = dt.year  - 1900;
    t.tm_mon  = dt.month - 1;
    t.tm_mday = dt.day;
    t.tm_hour = dt.hours;
    t.tm_min  = dt.minutes;
    t.tm_sec  = dt.seconds;
    t.tm_isdst = -1;
    time_t utc = mktime(&t);
    // mktime interprète en TZ locale → on force UTC d'abord
    // Simple workaround : utiliser localtime sur l'epoch système
    time_t now;
    time(&now);
    struct tm local;
    localtime_r(&now, &local);
    return local;
}

void rtc_set_alarm(time_t epoch) {
    struct tm t;
    gmtime_r(&epoch, &t);
    // SensorLib PCF85063 alarm : minute + heure (day alarm)
    rtc.setAlarm(t.tm_min, t.tm_hour, t.tm_mday, -1);
    rtc.enableAlarm();
    Serial.printf("[HAL] RTC alarm set at %02d:%02d (UTC)\n", t.tm_hour, t.tm_min);
}

void rtc_clear_alarm() {
    rtc.disableAlarm();
    rtc.clearAlarm();
}
