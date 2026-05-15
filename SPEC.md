# CompagnonV2 — Spécification unifiée

> Version v1 — mai 2026
> Fusion fonctionnelle de Compagnon v1 (PWA + firmware) et Compagnon2 (voice OS + agents mimiclaw)

---

## 1. Objectifs

1. Conserver :
   - les apps existantes du Compagnon (Nestor launcher, Radars, Bourse, Météo) et la logique d’OS + PWA activable.
   - la PWA Nestor telle qu’elle existe (agents, hub, TTS cascade, recherche, radars, bourse, météo, compagnon BLE).
2. Intégrer :
   - le design "Compagnon2" : wake word, STT/TTS, agent brain mimiclaw, mémoire hiérarchique L0-L4, auto-cristallisation, Jardinier, Fabrique.
   - un wake word permettant de lancer une app + une action dans l’app, y compris depuis light-sleep.
   - une app **Rappels** vocale + PWA (création de rappels, planification, notifications vocales).
3. Consolider l’architecture :
   - FreeRTOS + dual-core bien structuré : UI/IO sur un core, agents/LLM/logique sur l’autre.
   - HAL propre (display, touch, PMU, IMU, audio, BLE, WiFi, RTC, stockage).
   - plusieurs niveaux de stockage : NVS, FATFS flash, SD éventuelle, PWA (IndexedDB/localStorage).
   - BLE multi-caractéristiques cohérent avec la PWA existante.

---

## 2. Structure du repo CompagnonV2

```text
CompagnonV2/
├── README.md
├── SPEC.md                  # ce fichier — spéc générale
├── ARCHITECTURE.md          # détails modules + mapping FreeRTOS
├── pwa/                     # copie/adaptation de Nestor PWA (Compagnon v1)
│   ├── index.html
│   ├── manifest.json
│   ├── service-worker.js
│   ├── css/
│   └── src/
│       ├── app.js
│       ├── api/
│       ├── bt/
│       ├── core/
│       ├── device/
│       ├── input/
│       ├── storage/
│       ├── sync/
│       └── ui/
└── firmware/
    ├── CMakeLists.txt ou platformio.ini
    ├── partitions/
    ├── sdkconfig.defaults.esp32s3
    └── src/
        ├── main.cpp           # entry ESP-IDF/Arduino
        ├── config/
        ├── hal/
        ├── net/
        ├── system/
        ├── voice/
        ├── agent_brain/
        ├── apps/
        └── ui/
```

- `pwa/` : reprend l’arborescence et le code de `Compagnon` (Nestor PWA) presque tel quel.
- `firmware/` : reprend la logique de `compagnon/` (Compagnon v1) et la SPEC de `Compagnon2`.

---

## 3. OS ESP32 — Kernel & tâches FreeRTOS

### 3.1 Vue d’ensemble

- OS tournant sur ESP32-S3 dual-core.
- Usage recommandé :
  - **Core 0** : tâches bas niveau temps-réel (audio I/O, wake word ESP-SR, BLE, WiFi, timers).
  - **Core 1** : LVGL, UI, agent brain, orchestration, apps.
- Chaque app (Nestor, Radars, Bourse, Météo, Rappels) est un module qui expose :
  - `void app_init()`
  - `void app_start()`
  - `void app_stop()`
  - `void app_tick()` (optionnel, appelé par l’orchestrateur)

### 3.2 Tâches principales

Tâches FreeRTOS envisagées :

1. **task_ui_lvgl (Core 1)**
   - boucle : `lv_timer_handler()` + traitement input touch/boutons.
   - priorité haute.

2. **task_os_main (Core 1)**
   - appelle : `hal_pmu_tick()`, `wifi_mgr_tick()`, `ble_mgr_tick()`, `status_bar_tick()`, `agent_scheduler_tick()`, `app_orchestrator_tick()`.
   - gère la logique d’activation/suspension d’app.

3. **task_voice_io (Core 0)**
   - capture audio I2S (mic) pour wake word + STT.
   - joue TTS via I2S DAC.
   - gère les buffers DMA et la communication vers STT/TTS Groq.

4. **task_ble (Core 0)**
   - serveurs GATT, writes/notifications.
   - gestion GPS, LLM_RELAY, WIFI_PROVISION, AGENT_SYNC, TEXT_INPUT, DEVICE_STATUS.

5. **task_network (Core 0)**
   - WiFi connect/reconnect, NTP/RTC sync, OTA.

6. **task_agent_brain (Core 1)**
   - boucle ReAct : planification, tool calls, lecture/écriture mémoire L0-L4.
   - lance les requêtes LLM (via HTTP Groq) ou délègue au smartphone via LLM_RELAY.

Les stacks sont dimensionnées pour que **les apps non actives n’occupent pas de RAM dynamique** au-delà de leur code + quelques structures statiques. L’idée :
- UI du launcher + status bar toujours en mémoire.
- Chaque app charge juste son UI et ses quelques buffers quand elle est lancée, puis libère tout dans `app_stop()`.

---

## 4. Gestion réseau : WiFi, BLE, fallback

### 4.1 WiFi

- Basé sur `wifi_mgr` existant :
  - WiFiManager non-bloquant pour provisioning initial (AP "Compagnon_Setup").
  - Reconnexion automatique, timeout portail 180s.
- Niveaux :
  - **WiFi FULL** : LLM, STT, TTS, APIs météo/bourse, sync PWA.
  - **WiFi PARTIAL** : seulement certaines APIs critiques (météo, rappels sync...).
  - **NO WIFI** : fallback BLE relay.

### 4.2 BLE multi-caractéristiques

Côté firmware, implémenter **toutes** les caractéristiques attendues par la PWA :

- Service UUID : `4FAFC201-1FB5-459E-8FCC-C5C9C331914B` (ou équivalent unique), avec characteristics :
  - `GPS` — existant — format `"lat,lon"`.
  - `WIFI_SCAN` — écritures PWA → réponses JSON listant SSID.
  - `WIFI_PROVISION` — écritures PWA `{ ssid, password }` → connect.
  - `AGENT_SYNC` — protocole JSON déjà défini (get_agents, push, config...).
  - `TEXT_INPUT` — reçoit texte PWA, injecte dans l’app courante (Nestor, Rappels...).
  - `LLM_RELAY` — envoie requêtes LLM → reçoit réponse.
  - `DEVICE_STATUS` — expose batterie, WiFi, mode silencieux, summary OS.

### 4.3 Fallback HTTP over BLE

- En absence de WiFi, les appels LLM/STT/TTS sont sérialisés en JSON et envoyés sur `LLM_RELAY`.
- La PWA reçoit ces requêtes, appelle Groq/Gemini/Perplexity, renvoie le résultat.
- Le firmware ne fait plus d’HTTP direct dans ce mode.

---

## 5. UI LVGL : launcher + status bar

### 5.1 Launcher carousel

- Reprise du tileview 4 apps de Compagnon v1, étendu à 5 apps avec **Rappels**.
- Chaque carte contient :
  - icône LVGL.
  - titre (Nestor, Radars, Bourse, Météo, Rappels).
  - sous-titre.
- Les boutons physiques LEFT/RIGHT naviguent, appui long RIGHT lance l’app.

### 5.2 Barre de statut

- En haut, 480×36 px (ou 460×36 selon résolution exacte).
- Affiche :
  - Date et heure format `"dd mmm · HH:MM"`.
  - Icône BLE allumé si appairé.
  - Icône WiFi allumé si connecté.
  - Jauge batterie 28×12 avec couleur selon pourcentage, pourcentage au centre.
- Couleurs batterie :
  - > 30% → vert.
  - 15–30% → orange.
  - < 15% → rouge.

---

## 6. Wake word, STT/TTS et intégration apps

### 6.1 Wake word

- Utiliser ESP-SR (ou équivalent) pour un mot clé type "Nestor".
- Au réveil :
  - Sortir de light sleep si nécessaire.
  - Allumer écran si éteint (sauf mode silencieux + écran éteint explicitement ? configurable).
  - Afficher une vue "listening" (waveform).

### 6.2 Commandes vocales globales

Une fois wake word détecté, l’utterance est transcrite via :
- STT Groq Whisper (WiFi/LLM direct).
- ou fallback BLE → PWA → STT (via Web Speech ou Groq Whisper côté téléphone).

Le texte est envoyé à l’**orchestrateur** qui :
- décide si c’est une commande d’app ("ouvre radars", "montre la météo", "rappelle-moi...").
- route vers l’agent approprié (Rappels, Radars, Bourse...).

### 6.3 Bouton micro LVGL

- Dans certaines vues (Nestor, Rappels), un bouton micro :
  - clique/touch = déclencher la même pipeline que le wake word, mais sans phrase clé.
  - utile quand l’OS est déjà éveillé.

### 6.4 TTS OS

- Utiliser Groq PlayAI TTS en priorité, avec fallback locaux (sons basiques) si offline.
- TTS utilisé pour :
  - confirmations ("Très bien, je te rappellerai demain à 9h"),
  - erreurs ("Je n’ai pas réussi à contacter Groq"),
  - alarmes de rappel.

- Mode silencieux global :
  - variable persistée (NVS/FATFS + sync PWA),
  - si actif, l’OS ne joue que des sons minimaux (vibration si hardware) ou rien.

---

## 7. App Rappels

### 7.1 Objectifs

- Créer des rappels soit :
  - depuis la PWA (input texte + date/heure + avance).
  - soit par la voix via wake word ou bouton micro.
- L’ESP32 doit :
  - se réveiller à l’heure du rappel.
  - jouer un son (si pas en silencieux).
  - prononcer le texte du rappel.

### 7.2 Modèle de données des rappels

Stockage principal côté ESP32 dans FATFS (JSON, par exemple `reminders.json`) :

```json
[{
  "id": "uuid",
  "title": "RDV dentiste",
  "description": "Dentiste", // optionnel
  "datetime": "2026-05-20T09:00:00+02:00",
  "advance_minutes": 15,
  "status": "scheduled|done|cancelled",
  "created_at": "...",
  "updated_at": "..."
}]
```

- Les rappels sont indexés en mémoire pour planifier les alarms.
- Un fichier de log peut être conservé pour l’historique ("déjà rappelé").

### 7.3 Création depuis la PWA

- Dans la vue "Rappels" PWA :
  - un formulaire : titre, description, date, heure, avance (liste : 0/5/15/30/60 min...).
  - envoi via BLE AGENT_SYNC ou une caractéristique dédiée (ex : `REMINDERS_SYNC`), mais idéalement réutiliser AGENT_SYNC.

### 7.4 Création par la voix

- Wake word + STT → texte comme "rappelle-moi de prendre mes médicaments demain à 8 heures".
- Un agent dédié (Rappels) :
  - parse la phrase (date/heure, contenu).
  - si ambigu (pas d’heure), demande une précision par TTS.
  - confirme par TTS une fois enregistré.

### 7.5 Scheduling et réveil

- L’OS calcule le prochain rappel à venir (datetime − advance_minutes).
- Utiliser :
  - soit RTC + wakeup timer (si puce RTC type PCF85063 ou RTC interne ESP32).
  - soit timers FreeRTOS + light sleep avec `esp_sleep_enable_timer_wakeup`.
- Au réveil :
  - son + TTS.
  - UI affiche le rappel.
  - possibilité de "remettre plus tard" ou "marquer comme fait".

### 7.6 Synchronisation avec PWA

- La PWA doit pouvoir :
  - lire la liste des rappels.
  - en créer/éditer/supprimer.
  - recevoir des events (rappel déclenché, complet, snooze...).
- Recommandation :
  - exposer dans AGENT_SYNC des commandes `get_reminders`, `set_reminders`, `reminder_event`.

---

## 8. Mémoire hiérarchique & agents Nestor

- Reprise du modèle L0-L4 de Compagnon2.
- Mapping stockage :
  - `/fatfs/system/L0.json` : contraintes système.
  - `/fatfs/system/skill_index.json` : index de skills.
  - `/fatfs/memory/MEMORY.md` : faits utilisateur.
  - `/fatfs/skills/auto/*.json` : skills auto-cristallisées.
  - `/fatfs/sessions/*.json` : logs de sessions.
- Les agents Nestor (orchestrateur, jardinier, fabrique, etc.) sont définis en JSON dans `/fatfs/agents/`.
- La PWA garde sa propre copie pour l’UI, mais la source de vérité long terme est côté ESP32 (ou l’inverse, à décider clairement dans ARCHITECTURE.md).

---

## 9. Orchestration améliorée (Nestor core)

- Conserver la logique de `orchestrator-engine.js` pour la PWA, mais :
  - ajouter la notion de tâches planifiées (cron-like) pour Jardinier, Rappels, refresh radars/bourse/météo.
  - permettre la cristallisation de nouveaux agents (skills) à partir de workflows récurrents.
- Côté firmware :
  - implémenter un agent brain mimiclaw inspiré de Compagnon2 :
    - ReAct loop.
    - création/maintenance de skills.
    - usage des mêmes agents que la PWA autant que possible.

---

## 10. Stockage multi-niveaux

1. **NVS** : flags, config critique (WiFi, mode silencieux, préférences essentielles).
2. **FATFS flash** : agents, mémoire, rappels, sessions, skills auto, logs.
3. **SD card** (optionnel) : archives longues, logs debug.
4. **PWA (IndexedDB/localStorage)** : cache agents, historique UI, clés API.

Une stratégie de sync doit être définie pour éviter les conflits entre PWA et firmware.

---

## 11. Roadmap CompagnonV2

1. Porter le HAL + LVGL de Compagnon v1 dans `firmware/`.
2. Implémenter la partition table Compagnon2 + FATFS.
3. Implémenter les tâches FreeRTOS de base (UI, OS main, BLE, network).
4. Implémenter BLE multi-caractéristiques complet.
5. Rebrancher les apps Nestor/Radars/Bourse/Météo existantes.
6. Ajouter l’app Rappels (PWA + firmware + agent vocal).
7. Intégrer wake word + STT/TTS Groq + orchestrateur vocal.
8. Intégrer agent brain mimiclaw, Jardinier, Fabrique.
9. Ajouter planification avancée des tâches (cron) pour agents et rappels.
