# CompagnonV2 — Compagnon OS unifié

Ce repo fusionne **Compagnon** (PWA + firmware ESP32-S3) et **Compagnon2** (voice OS + agents mimiclaw) en un **OS Compagnon** consolidé et exhaustif.

## Ce que c'est

- **Compagnon** = le projet. L'OS s'appelle Compagnon OS, la PWA s'appelle Compagnon PWA.
- **Nestor** = une APP parmi les 8 apps du projet, pas l'OS ni la PWA.
- Les 8 apps existent à la fois sur le device ESP32 et dans la PWA.

## Les 8 apps

| # | App | PWA | Firmware |
|---|-----|-----|----------|
| 1 | Bourse | ✅ | ✅ |
| 2 | Météo | ✅ | ✅ |
| 3 | Nestor | ✅ | ✅ |
| 4 | Musique | ✅ | ✅ |
| 5 | Radars | ✅ | ✅ |
| 6 | SmartHome | ✅ | ✅ |
| 7 | Ecovacs | ✅ | ✅ |
| 8 | Rappels | ✅ (à créer) | ✅ (à créer) |

## Architecture

```
CompagnonV2/
├── README.md
├── SPEC.md            # spécification fonctionnelle complète
├── ARCHITECTURE.md    # découpage modules, FreeRTOS, dual-core, BLE, sync
├── pwa/               # Compagnon PWA (responsive, standalone, config ESP32)
└── firmware/          # Compagnon OS (ESP32-S3, FreeRTOS, LVGL, voice, agents)
```

## Points clés

- **Standalone de chaque côté** : PWA tourne seule (sans ESP32), firmware tourne seul (sans téléphone).
- **Sync bidirectionnelle** : agents, rappels, mémoire, configs se synchronisent dans les deux sens via BLE.
- **WiFi provisioning** : toujours via la PWA (scan + saisie mdp + envoi BLE → NVS ESP32).
- **Voice** : wake word (ESP-SR), STT/TTS Groq, bouton micro LVGL, mode silencieux global.
- **Agent brain** : mimiclaw (Compagnon2) fusionné avec orchestrateur PWA actuel — meilleur des deux.
- **SD card optionnelle** : cold storage si présente, mode dégradé si absente (sans blocage).
- **PWA responsive** : cible iPhone 13, adaptée tablette/Android.

Voir `SPEC.md` pour la spécification complète et `ARCHITECTURE.md` pour l'architecture détaillée.
