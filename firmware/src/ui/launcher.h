#pragma once
/*
 * ui/launcher.h — Launcher carousel LVGL CompagnonV2
 *
 * Apps disponibles (ordre du carousel) :
 *   0 : Nestor      (agent IA)
 *   1 : Rappels     (reminders)
 *   2 : SmartHome   (Tuya domotique)
 *   3 : Ecovacs     (robot aspirateur)
 *   4 : Radars      (futurs : radars météo, bourse...)
 *
 * Navigation :
 *   Bouton RIGHT court : app suivante
 *   Bouton LEFT  court : app précédente
 *   Bouton RIGHT long  : ouvrir app active → app_start()
 *   Touch swipe        : navigation directe
 *   Wake word          : ouvre l'app reconnue par l'orchestrateur vocal
 */

#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

void ui_launcher_init(void);
void ui_launcher_btn_tick(void);
void ui_launcher_open_app(int app_index);  // 0-based
void ui_launcher_close_app(void);
void ui_power_menu_show(void);

// Callback appelé par l'orchestrateur vocal pour ouvrir une app par nom
void ui_launcher_open_by_name(const char *app_name);

#ifdef __cplusplus
}
#endif
