#pragma once
// ============================================================
// sd_mgr — lecture/écriture fichiers sur SD via SPIFFS/FFat
// Utilisé par reminders_app pour persister /reminders/reminders.json
// ============================================================
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void  sd_mgr_init();

// Lit un fichier entier — retourne un buffer malloc()
// L'appelant doit appeler free() sur le résultat.
// Retourne nullptr si le fichier n'existe pas.
char* sd_mgr_read_file(const char* path);

// Écrit une chaîne dans un fichier (crée les dossiers si besoin)
bool  sd_mgr_write_file(const char* path, const char* content);

// Supprime un fichier
bool  sd_mgr_delete_file(const char* path);

// Vérifie si un fichier existe
bool  sd_mgr_exists(const char* path);

#ifdef __cplusplus
}
#endif
