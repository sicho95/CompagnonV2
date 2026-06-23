// ============================================================
// CompagnonV2 — system/os_kernel.h
// Kernel OS : lifecycle apps, routing vocal, scheduler rappels
// mode silencieux, power states
// ============================================================
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "../apps/app_base.h"

namespace os {

// ── Power states ────────────────────────────────────────────
enum class PowerState { ACTIVE, IDLE, LIGHT_SLEEP };

// ── App IDs ─────────────────────────────────────────────────
enum class AppId : uint8_t {
    NONE     = 0,
    NESTOR   = 1,
    RADARS   = 2,
    BOURSE   = 3,
    METEO    = 4,
    RAPPELS  = 5,
    DOMOTIQUE = 6,
    ECOVACS  = 7,
    COUNT
};

// ── App descriptor (enregistrement statique) ─────────────────
// Fix: structure alignée avec os_kernel.cpp :
//   - icon   : emoji ou nom icône
//   - instance : pointeur AppBase* (lifecycle init/onResume/onPause)
//   - handle_intent : lambda ou fn ptr (intent vocal)
//   - start/stop supprimés (remplacés par instance->init/onResume/onPause)
struct AppDesc {
    AppId       id;
    const char* icon;        // emoji ou nom icône
    AppBase*    instance;    // pointeur vers l'instance statique de l'app
    void  (*handle_intent)(const char* intent, const char* param);  // peut être nullptr
};

// ── Intent vocal (queue kernel) ──────────────────────────────
struct VoiceIntent {
    AppId       target_app;
    char        intent[48];   // ex: "create_reminder"
    char        param[128];   // texte STT brut
};

// ── API kernel ───────────────────────────────────────────────
void     kernel_init();
void     apps_register_all();          // appel boot, coût RAM ~0
bool     app_launch(AppId id);         // start + affiche
void     app_close_current();          // stop + retour launcher
AppId    app_current();
AppBase* app_get_instance(AppId id);   // fix: déclaration manquante

void   kernel_post_intent(const VoiceIntent& intent); // depuis task_voice
void   kernel_post_alarm(const char* label);          // depuis ISR/deep-sleep
void   kernel_set_silent(bool silent);                // mode silencieux
bool   kernel_is_silent();
void   kernel_set_time_valid(bool v);                 // heure ok ?
bool   kernel_time_is_valid();

void   kernel_schedule_next_reminder(); // programme sleep timer RTC
void   kernel_tick();                   // appelé par task_os_main

} // namespace os
