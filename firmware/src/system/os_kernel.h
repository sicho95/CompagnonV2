// ============================================================
// CompagnonV2 — system/os_kernel.h
// Kernel OS : lifecycle apps, routing vocal, scheduler rappels
// mode silencieux, power states
// ============================================================
#pragma once
#include <stdint.h>
#include <stdbool.h>

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
    COUNT
};

// ── App descriptor (enregistrement statique) ─────────────────
struct AppDesc {
    AppId       id;
    const char* name;
    const char* icon;        // emoji ou nom icône
    bool  (*start)();        // alloue LVGL + task FreeRTOS
    void  (*stop)();         // libère LVGL + task
    void  (*handle_intent)(const char* intent, const char* param);
};

// ── Intent vocal (queue kernel) ──────────────────────────────
struct VoiceIntent {
    AppId       target_app;
    char        intent[48];   // ex: "create_reminder"
    char        param[128];   // texte STT brut
};

// ── API kernel ───────────────────────────────────────────────
void   kernel_init();
void   apps_register_all();          // appel boot, coût RAM ~0
bool   app_launch(AppId id);         // start + affiche
void   app_close_current();          // stop + retour launcher
AppId  app_current();

void   kernel_post_intent(const VoiceIntent& intent); // depuis task_voice
void   kernel_set_silent(bool silent);                // mode silencieux
bool   kernel_is_silent();
void   kernel_set_time_valid(bool v);                 // heure ok ?
bool   kernel_time_is_valid();

void   kernel_schedule_next_reminder(); // programme sleep timer RTC
void   kernel_tick();                   // appelé par task_os_main

} // namespace os
