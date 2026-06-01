#pragma once
// ============================================================
// CompagnonV2 — ui/ui_dispatch.h
// File d'attente thread-safe pour les appels LVGL provenant
// de tâches autres que task_ui_lvgl.
// LVGL n'est PAS thread-safe : tout lv_scr_load*, lv_obj_*
// appelé hors de task_ui_lvgl doit passer par ui_dispatch_post.
// ============================================================
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <functional>

namespace ui {

using UiTask = std::function<void()>;

// Initialise la queue (appelé une seule fois dans task_ui_lvgl)
void dispatch_init();

// Poste une tâche depuis n'importe quel contexte (thread-safe)
// Retourne false si la queue est pleine
bool dispatch_post(UiTask fn);

// Exécute toutes les tâches en attente — à appeler dans task_ui_lvgl
void dispatch_flush();

} // namespace ui
