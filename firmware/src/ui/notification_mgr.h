#pragma once
// ============================================================
// CompagnonV2 — ui/notification_mgr.h
// Overlay toast LVGL (bannière en haut de l'écran)
// Appelé par os_kernel.cpp :
//   ui::notification_post(label, duration_ms)
//   ui::notification_tick()   — appel chaque kernel_tick()
// ============================================================
#include <Arduino.h>
#include <lvgl.h>

namespace ui {

/**
 * Affiche un toast (bannière semi-transparente) en haut de l'écran.
 * @param message  Texte à afficher (ex : label du rappel)
 * @param duration_ms  Durée d'affichage en ms (ex : 8000)
 * Si un toast est déjà actif, il est remplacé.
 */
void notification_post(const String& message, uint32_t duration_ms = 5000);

/**
 * À appeler dans kernel_tick() (~20 ms).
 * Gère l'expiration et le masquage automatique du toast.
 */
void notification_tick();

/**
 * Masque immédiatement le toast actif (si présent).
 */
void notification_dismiss();

} // namespace ui
