# Notes de portage — CompagnonV2

## Provenance des fichiers

| Fichier CompagnonV2 | Source | Statut |
|---|---|---|
| `CompagnonV2.ino` | `Compagnon/compagnon/compagnon.ino` | Porté + étendu |
| `src/hal/display.*` | `Compagnon/compagnon/src/hal/display.*` | **Copier tel quel** |
| `src/hal/touch.*` | `Compagnon/compagnon/src/hal/touch.*` | **Copier tel quel** |
| `src/hal/imu.*` | `Compagnon/compagnon/src/hal/imu.*` | **Copier tel quel** |
| `src/hal/pmu.*` | `Compagnon/compagnon/src/hal/pmu.*` | **Copier tel quel** |
| `src/hal/audio.*` | Nouveau | Squelette I2S |
| `src/system/wifi_mgr.*` | `Compagnon/compagnon/src/system/wifi_mgr.*` | **Copier tel quel** |
| `src/system/orchestrator.*` | `Compagnon/compagnon/src/system/orchestrator.*` | À étendre (dispatch vocal) |
| `src/system/sd_mgr.*` | `Compagnon/compagnon/src/system/sd_mgr.*` | **Copier tel quel** |
| `src/system/brain.*` | `Compagnon/compagnon/src/system/brain.*` | À étendre (ReAct) |
| `src/system/power_mgr.*` | Nouveau | Light/deep sleep |
| `src/net/ble_mgr.*` | `Compagnon/compagnon/src/net/ble_mgr.*` | **Copier tel quel** |
| `src/net/ota.*` | `Compagnon/compagnon/src/net/ota.*` | **Copier tel quel** |
| `src/config/nvs_config.*` | `Compagnon/compagnon/src/config/nvs_config.*` | Étendu (bool, u8, str) |
| `src/ui/launcher.*` | `Compagnon/compagnon/src/ui/launcher.*` | À étendre (+Rappels, wake word) |
| `src/ui/status_bar.*` | `Compagnon/compagnon/src/ui/status_bar.*` | À étendre (icône mic) |
| `src/voice/voice_engine.*` | Nouveau | Squelette wake word + STT/TTS |
| `src/apps/reminders/*` | Nouveau | App Rappels complète |
| `src/apps/smarthome/*` | `Compagnon/compagnon/src/apps/smarthome/*` | **Copier tel quel** |
| `src/apps/ecovacs/*` | `Compagnon/compagnon/src/apps/ecovacs/*` | **Copier tel quel** |

## Actions manuelles requises

1. **Copier** les fichiers HAL, net, system, apps existants depuis `Compagnon/compagnon/src/`
   (voir tableau ci-dessus — colonne "Copier tel quel").

2. **Pins audio** : vérifier les numéros GPIO I2S mic/codec dans `src/hal/audio.cpp`
   sur le schéma officiel Waveshare 2.16".

3. **ESP-SR WakeNet** : ajouter la dépendance dans `libraries.txt` ou Arduino Library Manager
   et décommenter les appels dans `src/voice/voice_engine.cpp`.

4. **TTS/STT Groq** : implémenter les stubs `stt_groq()` et `tts_speak()` dans
   `src/voice/voice_engine.cpp` (upload WAV → Groq Whisper, stream PCM → hal_audio_play).

5. **NVS étendu** : ajouter `nvs_set_bool`, `nvs_get_bool`, `nvs_set_u8`, `nvs_get_u8`,
   `nvs_set_str`, `nvs_get_str` dans `src/config/nvs_config.cpp`.

6. **Launcher** : ajouter l'app Rappels (index 1) dans le tileview LVGL de `launcher.cpp`
   et brancher `ui_launcher_open_by_name()` sur le dispatch vocal de l'orchestrateur.

7. **Light sleep** : décommenter `esp_light_sleep_start()` dans `power_mgr.cpp` quand
   la configuration wake-source (touch GPIO + RTC timer + wake word) est validée.

## Framework & LVGL

- **Arduino 3.3.8** (arduino-esp32)
- **LVGL 9.x**
- **ESP-IDF sous-jacent** : 5.1.x (embarqué dans arduino-esp32 3.3.8)
- Board : `esp32s3dev` ou profil Waveshare AMOLED 2.16
- Partition table : `huge_app.csv` (recommandé) ou custom avec OTA + FATFS

## Point de portage critique: CO5300 + CST9220 + QMI8658

Ne pas recopier une ancienne stratégie basée sur `Arduino_CO5300::setRotation(LCD_ROTATION)`, `setSwapXY(true)` ou une zone LVGL réduite `440x460`. L'état validé de ce firmware est documenté dans [WAVESHARE_2_16_BRINGUP.md](/Users/damien/Documents/Arduino/CompagnonV2/WAVESHARE_2_16_BRINGUP.md).

Résumé à respecter dans un nouveau projet:

- CO5300 en `setRotation(0)`
- `LCD_ROTATION` utilisé comme orientation de montage seulement
- rotation 90/270 faite dans le `flush_cb`
- LVGL en plein `480x480`
- safe area de layout distincte, ici `450x460`
- CST9220 sans swap/mirror SensorLib
- `touch.cpp` mappe les coordonnées selon `display_get_rotation()`
- une seule lecture indev explicite par boucle UI
- `lv_timer_handler()` passe avant les ticks gesture custom
- timer interne indev mis en pause
- `app_launch()` / `app_close_current()` sont sérialisés sur le thread UI
- si l'action vient d'un callback LVGL, `launch/close` est différé via `lv_async_call()`
- les transitions et `onResume()` ne doivent pas appeler `display_force_refresh()` / `lv_refr_now()`
- après `launch/close`, utiliser `display_request_refresh()` ; la task UI consomme ensuite la demande hors callback
- aucune app ne fait d'HTTP bloquant dans `init()` / `onResume()`
- QMI8658 filtré et converti avec offset de `180°`
