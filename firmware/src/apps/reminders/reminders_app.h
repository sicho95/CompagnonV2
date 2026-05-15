#pragma once
/*
 * apps/reminders/reminders_app.h — App Rappels CompagnonV2
 *
 * Fonctionnalités :
 *   - Stockage des rappels dans /sd/reminders.json (ou NVS si SD absent)
 *   - Scheduling : calcule le prochain wakeup RTC
 *   - Au réveil : son (si non silencieux) + TTS du rappel
 *   - UI LVGL : liste des rappels + formulaire création (clavier / vocal)
 *   - Sync BLE : get_reminders / set_reminders via agent_sync
 *   - Wake word : "rappelle-moi de X le Y à Z" → agent rappels
 *
 * Modèle de données (reminders.json) :
 *   [
 *     {
 *       "id": "uuid",
 *       "title": "Dentiste",
 *       "description": "RDV cabinet",
 *       "datetime": "2026-05-20T14:00:00",
 *       "advance_minutes": 30,
 *       "status": "scheduled",  // scheduled | done | cancelled | snoozed
 *       "snooze_minutes": 10,
 *       "created_at": "2026-05-15T10:00:00",
 *       "updated_at": "2026-05-15T10:00:00"
 *     }
 *   ]
 */

#ifdef __cplusplus
extern "C" {
#endif

void reminders_app_init(void);   // charge JSON + programme le prochain wakeup RTC
void reminders_app_tick(void);   // à appeler dans loop() — vérifie si un rappel sonne

// Cycle de vie app (appelé par le launcher)
void reminders_app_start(void);  // crée l'UI LVGL
void reminders_app_stop(void);   // détruit l'UI LVGL, libère RAM

// Sync BLE
void reminders_app_ble_set(const char *json_payload);  // reçoit un tableau de rappels
void reminders_app_ble_get(char *out, size_t out_len);  // sérialise les rappels en JSON

// Création depuis la voix (appelé par l'orchestrateur vocal)
void reminders_app_create_from_text(const char *natural_text);

#ifdef __cplusplus
}
#endif
