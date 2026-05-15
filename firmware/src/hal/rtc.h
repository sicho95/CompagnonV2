#pragma once
// ============================================================
// CompagnonV2 — HAL RTC
// Chip : PCF85063A (I2C, adresse 0x51)
// I2C  : partagé IIC_SDA/SCL
// Usage: heure/date persistante + alarme pour réveil rappels
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include "../config/pins.h"

#define PCF85063_ADDR  0x51

typedef struct {
    uint16_t year;    // ex. 2026
    uint8_t  month;  // 1–12
    uint8_t  day;    // 1–31
    uint8_t  hour;   // 0–23
    uint8_t  minute; // 0–59
    uint8_t  second; // 0–59
    uint8_t  wday;   // 0=dim, 1=lun, …, 6=sam
} rtc_datetime_t;

/**
 * @brief Initialise le PCF85063A.
 */
bool hal_rtc_init(void);

/**
 * @brief Lit la date/heure courante depuis le RTC.
 */
bool hal_rtc_get(rtc_datetime_t* dt);

/**
 * @brief Définit la date/heure (à appeler après NTP sync).
 */
bool hal_rtc_set(const rtc_datetime_t* dt);

/**
 * @brief Synchronise le RTC depuis l'epoch Unix (timestamp NTP).
 */
bool hal_rtc_set_from_epoch(time_t epoch);

/**
 * @brief Programme une alarme one-shot pour réveil (ex. rappel).
 *        L'ESP32 sortira du light sleep à cette heure.
 * @param epoch timestamp Unix de l'alarme
 */
bool hal_rtc_set_alarm(time_t epoch);

/**
 * @brief Efface l'alarme courante.
 */
void hal_rtc_clear_alarm(void);

/**
 * @brief Retourne un timestamp epoch depuis la date/heure RTC.
 */
time_t hal_rtc_to_epoch(const rtc_datetime_t* dt);

/**
 * @brief Formate la date en français pour la status bar.
 *        Format : "15 mai 2026 - 16:05"
 * @param dt  date/heure source
 * @param buf buffer destination (min 32 chars)
 */
void hal_rtc_format_fr(const rtc_datetime_t* dt, char* buf, size_t buflen);
