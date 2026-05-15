# CompagnonV2 — Architecture détaillée

Ce document précise davantage comment organiser le code entre firmware ESP32-S3 et PWA.

## 1. Firmware ESP32-S3

Arborescence proposée :

```text
firmware/
├── CMakeLists.txt
├── partitions/
│   └── compagnon2_16mb.csv
├── sdkconfig.defaults.esp32s3
└── src/
    ├── main.cpp
    ├── config/
    ├── hal/
    ├── net/
    ├── system/
    ├── voice/
    ├── agent_brain/
    ├── storage/
    ├── apps/
    └── ui/
```

- `config/` : pins, secrets (template), lv_conf, kconfig-like.
- `hal/` : display, touch, pmu, imu, rtc, audio, sd.
- `net/` : wifi_mgr, ble_mgr, ota, http_client.
- `system/` : orchestrator, power manager, scheduler, time manager.
- `voice/` : wake word, ASR, TTS.
- `agent_brain/` : ReAct engine, tools, agents loader, memory L0-L4.
- `storage/` : NVS, FATFS mounts, JSON IO, reminders storage.
- `apps/` : nestor, radars, bourse, meteo, rappels.
- `ui/` : launcher, status_bar, app-specific UI.

## 2. PWA

Arborescence sous `pwa/` identique au repo Compagnon actuel.
