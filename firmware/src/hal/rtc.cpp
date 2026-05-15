// ============================================================
// CompagnonV2 — hal/rtc.cpp
// PCF85063 I2C + sync NTP
// Format FR : "15 mai 2026 - 16:05"
// ============================================================
#include "rtc.h"

static const char* MONTH_FR[] = {
    "", "janv.", "févr.", "mars", "avr.", "mai", "juin",
    "juil.", "août", "sept.", "oct.", "nov.", "déc."
};

// ── BCD helpers ───────────────────────────────────────────────────────────
static uint8_t _bcd2dec(uint8_t b) { return (b >> 4) * 10 + (b & 0x0F); }
static uint8_t _dec2bcd(uint8_t d) { return ((d / 10) << 4) | (d % 10); }

// ── Lecture registres PCF85063 ────────────────────────────────────────────
static bool _pcf_read(struct tm* t) {
    Wire.beginTransmission(PCF85063_ADDR);
    Wire.write(PCF85063_REG_SEC);
    if (Wire.endTransmission(false) != 0) return false;
    Wire.requestFrom(PCF85063_ADDR, (uint8_t)7);
    if (Wire.available() < 7) return false;

    t->tm_sec  = _bcd2dec(Wire.read() & 0x7F);
    t->tm_min  = _bcd2dec(Wire.read() & 0x7F);
    t->tm_hour = _bcd2dec(Wire.read() & 0x3F);
    Wire.read();                               // jour semaine
    t->tm_mday = _bcd2dec(Wire.read() & 0x3F);
    t->tm_mon  = _bcd2dec(Wire.read() & 0x1F) - 1;
    t->tm_year = _bcd2dec(Wire.read()) + 100; // base 1900
    t->tm_isdst = -1;
    return true;
}

// ── Écriture PCF85063 ────────────────────────────────────────────────────
static void _pcf_write(const struct tm* t) {
    Wire.beginTransmission(PCF85063_ADDR);
    Wire.write(PCF85063_REG_SEC);
    Wire.write(_dec2bcd(t->tm_sec));
    Wire.write(_dec2bcd(t->tm_min));
    Wire.write(_dec2bcd(t->tm_hour));
    Wire.write(0x00);                           // jour semaine
    Wire.write(_dec2bcd(t->tm_mday));
    Wire.write(_dec2bcd(t->tm_mon + 1));
    Wire.write(_dec2bcd(t->tm_year - 100));
    Wire.endTransmission();
}

// ── Init ──────────────────────────────────────────────────────────────────
bool rtc_init() {
    // I2C déjà démarré par touch_init()
    Wire.beginTransmission(PCF85063_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("[RTC] ERREUR: PCF85063 non détecté");
        return false;
    }
    // Reset control register (OS bit clear, clock running)
    Wire.beginTransmission(PCF85063_ADDR);
    Wire.write(PCF85063_REG_CTRL);
    Wire.write(0x00);
    Wire.endTransmission();

    // Charge l'heure RTC dans le système (struct timeval)
    struct tm t;
    if (_pcf_read(&t)) {
        time_t ts = mktime(&t);
        struct timeval tv = { .tv_sec = ts };
        settimeofday(&tv, nullptr);
        Serial.println("[RTC] PCF85063 init OK — heure restaurée");
    } else {
        Serial.println("[RTC] PCF85063 init OK — heure non lisible");
    }
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    return true;
}

// ── Sync NTP → RTC ───────────────────────────────────────────────────────
void rtc_set_from_ntp() {
    configTzTime("CET-1CEST,M3.5.0,M10.5.0/3",
                 "pool.ntp.org", "time.cloudflare.com", nullptr);
    delay(500);  // laisser le temps à NTP
    time_t now = time(nullptr);
    if (now < 1700000000UL) {
        Serial.println("[RTC] NTP sync échoué (time invalid)");
        return;
    }
    struct tm t;
    localtime_r(&now, &t);
    _pcf_write(&t);
    Serial.println("[RTC] NTP sync OK → PCF85063 mis à jour");
}

// ── Getter timestamp ─────────────────────────────────────────────────────
time_t rtc_get_time() { return time(nullptr); }

// ── Getter formaté FR ────────────────────────────────────────────────────
void rtc_get_local_fr(char* buf, size_t buf_len) {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    snprintf(buf, buf_len, "%d %s %d - %02d:%02d",
             t.tm_mday, MONTH_FR[t.tm_mon + 1],
             1900 + t.tm_year, t.tm_hour, t.tm_min);
}
