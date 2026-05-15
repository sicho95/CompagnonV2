#pragma once
// =============================================================
// CompagnonV2 — hal/rtc.h
// Driver : PCF85063 RTC I2C (via SensorLib)
// Rôle   : heure locale, wakeup timer pour rappels
// =============================================================
#include <Wire.h>
#include <SensorLib.h>
#include "../config/pins.h"

extern SensorPCF85063 rtc;

/**
 * @brief Initialise le PCF85063.
 * @return true si RTC répond.
 */
bool rtc_init();

/**
 * @brief Synchronise la RTC depuis NTP (appeler après WiFi connect).
 * @param epoch  timestamp Unix UTC.
 */
void rtc_sync_from_ntp(time_t epoch);

/**
 * @brief Retourne l'heure locale comme struct tm (TZ Europe/Paris).
 */
struct tm rtc_get_local_time();

/**
 * @brief Programme une alarme one-shot pour le réveil light-sleep.
 * @param epoch  timestamp Unix UTC du réveil souhaité.
 */
void rtc_set_alarm(time_t epoch);

/** @brief Annule l'alarme en cours. */
void rtc_clear_alarm();
