#pragma once
// ============================================================
// Orchestrateur CompagnonV2
// Coordonne les modules app, voice, net, reminders
// ============================================================

#ifdef __cplusplus
extern "C" {
#endif

void orchestrator_init();
void orchestrator_tick();

#ifdef __cplusplus
}
#endif
