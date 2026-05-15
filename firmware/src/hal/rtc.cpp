// ============================================================
// CompagnonV2 — HAL RTC — PCF85063A I2C
// ============================================================
#include "rtc.h"
#include <time.h>

// Registres PCF85063A
#define REG_CTRL1     0x00
#define REG_CTRL2     0x01
#define REG_SECONDS   0x04
#define REG_MINUTES   0x05
#define REG_HOURS     0x06
#define REG_DAYS      0x07
#define REG_WEEKDAYS  0x08
#define REG_MONTHS    0x09
#define REG_YEARS     0x0A
#define REG_ALARM_S   0x0B

static uint8_t bcd2dec(uint8_t bcd) { return (bcd >> 4) * 10 + (bcd & 0x0F); }
static uint8_t dec2bcd(uint8_t dec) { return ((dec / 10) << 4) | (dec % 10); }

static bool pcf_write(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(PCF85063_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

static uint8_t pcf_read(uint8_t reg) {
    Wire.beginTransmission(PCF85063_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)PCF85063_ADDR, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0;
}

bool hal_rtc_init(void) {
    Wire.beginTransmission(PCF85063_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("[HAL] RTC PCF85063 NOT found!");
        return false;
    }
    // Stop oscillator, clear STOP bit
    pcf_write(REG_CTRL1, 0x00);
    Serial.println("[HAL] RTC PCF85063A init OK");
    return true;
}

bool hal_rtc_get(rtc_datetime_t* dt) {
    Wire.beginTransmission(PCF85063_ADDR);
    Wire.write(REG_SECONDS);
    if (Wire.endTransmission(false) != 0) return false;
    Wire.requestFrom((uint8_t)PCF85063_ADDR, (uint8_t)7);
    if (Wire.available() < 7) return false;

    dt->second = bcd2dec(Wire.read() & 0x7F);
    dt->minute = bcd2dec(Wire.read() & 0x7F);
    dt->hour   = bcd2dec(Wire.read() & 0x3F);
    dt->day    = bcd2dec(Wire.read() & 0x3F);
    dt->wday   = Wire.read() & 0x07;
    dt->month  = bcd2dec(Wire.read() & 0x1F);
    dt->year   = 2000 + bcd2dec(Wire.read());
    return true;
}

bool hal_rtc_set(const rtc_datetime_t* dt) {
    Wire.beginTransmission(PCF85063_ADDR);
    Wire.write(REG_SECONDS);
    Wire.write(dec2bcd(dt->second));
    Wire.write(dec2bcd(dt->minute));
    Wire.write(dec2bcd(dt->hour));
    Wire.write(dec2bcd(dt->day));
    Wire.write(dt->wday & 0x07);
    Wire.write(dec2bcd(dt->month));
    Wire.write(dec2bcd((uint8_t)(dt->year - 2000)));
    return Wire.endTransmission() == 0;
}

bool hal_rtc_set_from_epoch(time_t epoch) {
    struct tm* t = localtime(&epoch);
    if (!t) return false;
    rtc_datetime_t dt;
    dt.second = t->tm_sec;
    dt.minute = t->tm_min;
    dt.hour   = t->tm_hour;
    dt.day    = t->tm_mday;
    dt.month  = t->tm_mon + 1;
    dt.year   = t->tm_year + 1900;
    dt.wday   = t->tm_wday;
    return hal_rtc_set(&dt);
}

time_t hal_rtc_to_epoch(const rtc_datetime_t* dt) {
    struct tm t = {};
    t.tm_sec  = dt->second;
    t.tm_min  = dt->minute;
    t.tm_hour = dt->hour;
    t.tm_mday = dt->day;
    t.tm_mon  = dt->month - 1;
    t.tm_year = dt->year - 1900;
    return mktime(&t);
}

bool hal_rtc_set_alarm(time_t epoch) {
    struct tm* t = localtime(&epoch);
    if (!t) return false;
    // PCF85063A alarm : minutes + heures + jours
    pcf_write(REG_ALARM_S,     dec2bcd(t->tm_sec));               // seconds alarm
    pcf_write(REG_ALARM_S + 1, dec2bcd(t->tm_min));               // minutes alarm
    pcf_write(REG_ALARM_S + 2, dec2bcd(t->tm_hour));              // hours alarm
    pcf_write(REG_ALARM_S + 3, dec2bcd(t->tm_mday));              // days alarm
    // Activer l'interruption alarme
    uint8_t ctrl2 = pcf_read(REG_CTRL2);
    pcf_write(REG_CTRL2, ctrl2 | 0x80); // AIE = 1
    return true;
}

void hal_rtc_clear_alarm(void) {
    uint8_t ctrl2 = pcf_read(REG_CTRL2);
    pcf_write(REG_CTRL2, ctrl2 & ~0x80); // AIE = 0
    pcf_write(REG_CTRL2, ctrl2 & ~0x08); // AF = 0 (clear flag)
}

// Mois en français
static const char* MONTHS_FR[] = {
    "", "jan", "fév", "mar", "avr", "mai", "jun",
    "jul", "aoû", "sep", "oct", "nov", "déc"
};

void hal_rtc_format_fr(const rtc_datetime_t* dt, char* buf, size_t buflen) {
    snprintf(buf, buflen, "%02d %s %04d - %02d:%02d",
             dt->day,
             (dt->month >= 1 && dt->month <= 12) ? MONTHS_FR[dt->month] : "?",
             dt->year,
             dt->hour,
             dt->minute);
}
