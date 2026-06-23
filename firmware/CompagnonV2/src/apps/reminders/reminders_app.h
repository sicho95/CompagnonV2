#pragma once
// API publique reminders_app
// Appelée depuis CompagnonV2.ino et depuis le pont BLE
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void reminders_app_init();
void reminders_app_tick();
void reminders_app_start();
void reminders_app_stop();

// Pont BLE → sync bidirectionnel
void reminders_app_ble_set(const char* json_payload);
void reminders_app_ble_get(char* out, size_t out_len);

// Création depuis reconnaissance vocale
void reminders_app_create_from_text(const char* natural_text);

#ifdef __cplusplus
}
#endif
