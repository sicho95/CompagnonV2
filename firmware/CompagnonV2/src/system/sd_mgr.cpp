// ============================================================
// sd_mgr — stockage fichiers via FFat (partition FATFS)
// Montage sur /ffat  —  Remplace l'ancien SPIFFS
// ============================================================
#include "sd_mgr.h"
#include <FFat.h>
#include <Arduino.h>
#include <cstring>
#include <cstdlib>

#define MOUNT_POINT "/ffat"
#define MAX_FILE_SIZE (64 * 1024)  // 64 KB max par fichier

static bool _mounted = false;

// Crée les dossiers parents si nécessaire
static void _mkdirs(const char* path) {
    char tmp[128];
    strncpy(tmp, path, sizeof(tmp) - 1);
    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            FFat.mkdir(tmp);
            *p = '/';
        }
    }
}

void sd_mgr_init() {
    if (!FFat.begin(true)) {   // true = formatOnFail
        Serial.println("[SD_MGR] FFat mount failed");
        _mounted = false;
        return;
    }
    _mounted = true;
    Serial.printf("[SD_MGR] FFat OK — free: %u KB\n",
                  (unsigned)(FFat.freeBytes() / 1024));
}

char* sd_mgr_read_file(const char* path) {
    if (!_mounted) return nullptr;
    char full[160];
    snprintf(full, sizeof(full), "%s%s", MOUNT_POINT, path);
    File f = FFat.open(full, FILE_READ);
    if (!f || f.isDirectory()) return nullptr;
    size_t sz = f.size();
    if (sz == 0 || sz > MAX_FILE_SIZE) { f.close(); return nullptr; }
    char* buf = (char*)malloc(sz + 1);
    if (!buf) { f.close(); return nullptr; }
    f.readBytes(buf, sz);
    buf[sz] = 0;
    f.close();
    return buf;
}

bool sd_mgr_write_file(const char* path, const char* content) {
    if (!_mounted || !content) return false;
    char full[160];
    snprintf(full, sizeof(full), "%s%s", MOUNT_POINT, path);
    _mkdirs(full);
    File f = FFat.open(full, FILE_WRITE);
    if (!f) return false;
    size_t written = f.print(content);
    f.close();
    return written > 0;
}

bool sd_mgr_delete_file(const char* path) {
    if (!_mounted) return false;
    char full[160];
    snprintf(full, sizeof(full), "%s%s", MOUNT_POINT, path);
    return FFat.remove(full);
}

bool sd_mgr_exists(const char* path) {
    if (!_mounted) return false;
    char full[160];
    snprintf(full, sizeof(full), "%s%s", MOUNT_POINT, path);
    return FFat.exists(full);
}
