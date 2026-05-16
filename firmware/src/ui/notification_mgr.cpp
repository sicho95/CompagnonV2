// ============================================================
// CompagnonV2 — ui/notification_mgr.cpp
// Overlay toast LVGL — bannière rappel/alarme
//
// Layout :
//   ┌──────────────────────────────────────┐  ← lv_obj, y=0, h=48px
//   │  🔔  <message>                        │  fond noir 80% alpha
//   └──────────────────────────────────────┘
//
// Thread-safety : notification_post() peut être appelé depuis n'importe
// quelle tâche via lv_async_call() pour rester sur le thread LVGL.
// ============================================================
#include "notification_mgr.h"
#include <Arduino.h>
#include <lvgl.h>

namespace ui {

// ── Etat interne ─────────────────────────────────────────────
static lv_obj_t*  _toast     = nullptr;  // conteneur principal
static lv_obj_t*  _label     = nullptr;  // label texte
static uint32_t   _expire_ms = 0;        // millis() cible pour masquage
static bool       _visible   = false;

// ── Données pour lv_async_call ───────────────────────────────
struct ToastReq {
    char     msg[128];
    uint32_t duration_ms;
};
static ToastReq _pending_req;

// ── Création lazy du widget ──────────────────────────────────
static void _ensure_widget() {
    if (_toast) return;

    lv_obj_t* scr = lv_scr_act();
    _toast = lv_obj_create(scr);

    // Géométrie : largeur plein écran, hauteur 48 px, ancré en haut
    lv_obj_set_size(_toast,
        lv_obj_get_width(scr),
        48);
    lv_obj_align(_toast, LV_ALIGN_TOP_MID, 0, 0);

    // Style fond semi-transparent
    lv_obj_set_style_bg_color(_toast,  lv_color_black(), 0);
    lv_obj_set_style_bg_opa(_toast,    LV_OPA_80, 0);
    lv_obj_set_style_border_width(_toast, 0, 0);
    lv_obj_set_style_pad_all(_toast,   8, 0);
    lv_obj_set_style_radius(_toast,    0, 0);
    lv_obj_add_flag(_toast, LV_OBJ_FLAG_FLOATING);  // passe au-dessus de tout

    // Label
    _label = lv_label_create(_toast);
    lv_obj_set_style_text_color(_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_label, &lv_font_montserrat_16, 0);
    lv_label_set_long_mode(_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(_label, lv_obj_get_width(scr) - 24);
    lv_obj_align(_label, LV_ALIGN_LEFT_MID, 8, 0);

    // Caché par défaut
    lv_obj_add_flag(_toast, LV_OBJ_FLAG_HIDDEN);
    _visible = false;
}

// ── Affichage (doit s'exécuter dans le thread LVGL) ──────────
static void _show_toast(void* user_data) {
    ToastReq* req = static_cast<ToastReq*>(user_data);
    _ensure_widget();

    lv_label_set_text(_label, req->msg);
    lv_obj_clear_flag(_toast, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(_toast);
    _expire_ms = millis() + req->duration_ms;
    _visible   = true;

    Serial.printf("[UI] Toast: %s (%lu ms)\n", req->msg, req->duration_ms);
}

// ── API publique ─────────────────────────────────────────────
void notification_post(const String& message, uint32_t duration_ms) {
    strncpy(_pending_req.msg, message.c_str(), sizeof(_pending_req.msg) - 1);
    _pending_req.msg[sizeof(_pending_req.msg) - 1] = '\0';
    _pending_req.duration_ms = duration_ms;
    // lv_async_call garantit l'exécution dans le thread LVGL
    lv_async_call(_show_toast, &_pending_req);
}

void notification_dismiss() {
    if (_toast && _visible) {
        lv_obj_add_flag(_toast, LV_OBJ_FLAG_HIDDEN);
        _visible   = false;
        _expire_ms = 0;
    }
}

void notification_tick() {
    if (!_visible) return;
    if (millis() >= _expire_ms) {
        notification_dismiss();
    }
}

} // namespace ui
