# CompagnonV2 — Configuration Arduino IDE

## Version requise
- **Arduino IDE** : 2.x (testé 2.3+)
- **ESP32 Arduino Core** : `3.3.8` (Espressif Systems)

## Réglages Tools (Board Manager)

| Option | Valeur |
|---|---|
| Board | `ESP32S3 Dev Module` |
| Flash Size | `16MB (128Mb)` |
| Flash Mode | `QIO 80MHz` |
| Partition Scheme | `Custom` → pointer sur `firmware/partitions.csv` |
| PSRAM | `OPI PSRAM` (8MB) |
| CPU Frequency | `240MHz` |
| USB Mode | `Hardware CDC and JTAG` |
| Upload Speed | `921600` |

## Comment utiliser une partition personnalisée dans Arduino IDE

1. Aller dans **Tools > Partition Scheme > Custom…**
2. Sélectionner le fichier `firmware/partitions.csv` de ce repo
   - Ou copier `partitions.csv` dans `~/Arduino/hardware/espressif/esp32/tools/partitions/`
   - Puis le sélectionner dans la liste avec son nom

## Bibliothèques à installer (Library Manager)

| Bibliothèque | Version |
|---|---|
| `LVGL` | `9.x` |
| `ArduinoJson` | `7.x` |
| `FFat` | incluse dans le core ESP32 |
| `WiFiClientSecure` | incluse dans le core ESP32 |
| `HTTPClient` | incluse dans le core ESP32 |
| `Preferences` | incluse dans le core ESP32 |

## Activer le bundle CA (WiFiClientSecure)

Dans Arduino IDE avec core 3.x, le bundle Mozilla est inclus automatiquement.
Aucune manipulation supplémentaire n'est nécessaire — `setCACertBundle()` fonctionne out of the box.

## Disposition flash

```
0x00000 — 0x08FFF   Bootloader
0x09000 — 0x0DFFF   NVS (20 KB)
0x0E000 — 0x0FFFF   OTA data
0x10000 — 0x50FFFF  app0 / ota_0  (5 MB)
0x51000 — 0xA0FFFF  app1 / ota_1  (5 MB)
0xA1000 — 0xA1FFFF  Core dump
0xA2000 — 0xA20FFF  NVS keys
0xA2100 — 0xFFFFFF  FATFS  (≈4 MB) ← /reminders.json ici
```

## PSRAM

Le projet utilise la PSRAM pour les buffers LVGL et les vecteurs TTS (jusqu'à ~500KB PCM).
Vérifier que `#define BOARD_HAS_PSRAM` est présent ou que PSRAM est activée dans les réglages du board.
