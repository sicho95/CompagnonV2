#pragma once
#include "pins.h"

// ============================================================
// CompagnonV2 — ui_config.h
// Constantes de mise en page partagées entre TOUS les modules UI.
//
// Toutes les apps, le launcher et la status bar doivent utiliser
// ces constantes pour rester dans la safe area et sous la status bar.
//
// Pour changer la résolution → modifier SCREEN_W / SCREEN_H.
// Pour ajuster les marges du boîtier arrondi → modifier BORDER_H / BORDER_V.
// ============================================================

// ─── Taille physique de l'écran ──────────────────────────────────────────────
#define SCREEN_W  480   // px — Waveshare AMOLED 2.16"
#define SCREEN_H  480   // px

// ─── Safe area — marges boîtier arrondi ──────────────────────────────────────
// BORDER_H : marge gauche ET droite (chacune)
// BORDER_V : marge haut ET bas (chacune, en plus de la status bar côté haut)
#define BORDER_H  15   // px — marge horizontale gauche/droite
#define BORDER_V  10   // px — marge verticale haut/bas

// ─── Zone d'affichage logique (centrée dans l'écran physique) ────────────────
// Origine absolue (coordonnées LVGL depuis coin haut-gauche de l'écran physique)
#define UI_X   BORDER_H                    //  15 px depuis la gauche
#define UI_Y   BORDER_V                    //  10 px depuis le haut
// Dimensions de la zone logique
#define UI_W   (SCREEN_W - 2 * BORDER_H)  // 480 - 30 = 450 px
#define UI_H   (SCREEN_H - 2 * BORDER_V)  // 480 - 20 = 460 px

// ─── Status bar ──────────────────────────────────────────────────────────────
// Placée en haut de la zone logique (UI_X, UI_Y), largeur UI_W
#define STATUS_BAR_H  36   // px — hauteur status bar

// ─── Zone des apps (sous la status bar, dans la safe area) ───────────────────
// APP_Y : coordonnée Y absolue où commence le contenu d'une app
// APP_H : hauteur disponible pour le contenu d'une app
#define APP_X   UI_X                        //  15 px
#define APP_Y   (UI_Y + STATUS_BAR_H)       //  10 + 36 = 46 px
#define APP_W   UI_W                        // 450 px
#define APP_H   (UI_H - STATUS_BAR_H)       // 460 - 36 = 424 px

// ─── Récapitulatif visuel ─────────────────────────────────────────────────────
// Ecran physique : 480 x 480 px
// Safe area      : x=15..465, y=10..470  (450 x 460 px)
// Status bar     : x=15, y=10, w=450, h=36
// Zone apps      : x=15, y=46, w=450, h=424
