# CompagnonV2 — Nestor OS unifié

Ce repo fusionne **Compagnon v1** (PWA + firmware Waveshare AMOLED 2.16") et **Compagnon2** (firmware xiaoclaw/mimiclaw) en un **OS Nestor** unique :

- OS ESP32-S3 dual-core avec FreeRTOS, LVGL launcher, barre de statut, gestion WiFi/BLE/OTA.
- Wake word + STT/TTS + agents Nestor embarqués avec mémoire hiérarchique L0-L4.
- Relais BLE vers la PWA Nestor (LLM relay, WiFi provisioning, sync agents, clavier, device status).
- PWA Nestor servant d’UI riche, de hub d’agents et de cerveau cloud quand disponible.
- Nouvelle app **Rappels** (Reminders) côté PWA + firmware.

Voir `SPEC.md` pour la spécification complète et `ARCHITECTURE.md` pour les décisions d’architecture.