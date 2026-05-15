# CompagnonV2 — Spécification fonctionnelle complète

> Version v2 — mai 2026  
> Fusion de Compagnon (PWA + firmware) et Compagnon2 (voice OS + agents mimiclaw)

---

## 1. Nomenclature et rôles

### 1.1 Définitions

- **Compagnon** = nom du projet dans son ensemble.
- **Compagnon OS** = le firmware ESP32-S3 (cet OS s'appelle Compagnon OS).
- **Compagnon PWA** = la Progressive Web App (UI, config, hub cloud).
- **Nestor** = une APP de Compagnon, l'assistant IA (1 parmi les 8 apps).
- **Compagnon device** = l'ESP32-S3 avec écran Waveshare AMOLED 2.16".

### 1.2 Les 8 apps (identiques PWA + firmware)

1. Bourse
2. Météo
3. Nestor (assistant IA embarqué)
4. Musique
5. Radars
6. SmartHome (Maison)
7. Ecovacs
8. Rappels ← *nouvelle app à créer*

### 1.3 Indépendance et sync

- La **PWA tourne seule** sans ESP32 : toutes les apps y fonctionnent en mode cloud (APIs directes, LLM Groq).
- Le **firmware tourne seul** sans téléphone : apps fonctionnent via WiFi direct, avec fallback BLE relay si pas de WiFi.
- La **sync bidirectionnelle** PWA ↔ ESP32 via BLE permet de :
  - Pousser depuis PWA → ESP32 : agents créés/cristallisés, rappels ajoutés depuis la PWA, configs d'apps, clés API.
  - Remonter depuis ESP32 → PWA : agents auto-cristallisés, mémoire L2, rappels créés vocalement, données d'apps (historique radars, alertes bourse…).

---

## 2. Structure du repo

```text
CompagnonV2/
├── README.md
├── SPEC.md
├── ARCHITECTURE.md
├── pwa/
│   ├── index.html
│   ├── manifest.json
│   ├── service-worker.js
│   ├── css/
│   │   ├── base.css
│   │   ├── layout.css
│   │   └── apps/         # CSS par app
│   └── src/
│       ├── app.js         # entry point, routing, init
│       ├── api/           # backends LLM, météo, bourse, etc.
│       ├── bt/            # BLE Web Bluetooth (ble.js, ble_protocol.js, ble_status.js)
│       ├── core/          # orchestrateur, agent engine, TTS cascade
│       ├── device/        # provisioning WiFi, device_settings, device_config
│       ├── input/         # bt_keyboard, voice input, text input
│       ├── storage/       # IndexedDB, localStorage wrappers
│       ├── sync/          # agents_sync, reminders_sync, memory_sync
│       └── ui/
│           ├── app-shell.js    # shell responsive (nav, bottom bar, header)
│           ├── launcher.js     # page d'accueil avec les 8 apps en cards
│           ├── compagnon.js    # vue config ESP32 (BLE, WiFi, agents, batterie)
│           ├── bourse-view.js
│           ├── meteo-view.js
│           ├── nestor-view.js
│           ├── musique-view.js
│           ├── radar-view.js
│           ├── smarthome.js
│           ├── ecovacs-view.js
│           └── rappels-view.js  # nouvelle
└── firmware/
    ├── platformio.ini        # ou CMakeLists.txt ESP-IDF
    ├── partitions/
    │   └── compagnon_16mb.csv
    ├── sdkconfig.defaults.esp32s3
    └── src/
        ├── main.cpp
        ├── config/           # pins.h, secrets_template.h, lv_conf.h
        ├── hal/              # display, touch, pmu, imu, rtc, audio_io, sd_card
        ├── net/              # wifi_mgr, ble_mgr, ota, http_client
        ├── system/           # os_main, power_mgr, scheduler, time_mgr, mode_mgr
        ├── voice/            # wake_word, asr, tts
        ├── agent_brain/      # react_engine, tools/, agents_loader, memory_l0_l4
        ├── storage/          # nvs_mgr, fatfs_mgr, sd_mgr, json_io, reminders_db
        ├── apps/
        │   ├── app_base.h
        │   ├── app_registry.h/.cpp
        │   ├── bourse/
        │   ├── meteo/
        │   ├── nestor/
        │   ├── musique/
        │   ├── radars/
        │   ├── smarthome/
        │   ├── ecovacs/
        │   └── rappels/      # nouvelle
        └── ui/
            ├── launcher.h/.cpp
            ├── status_bar.h/.cpp
            └── apps/         # UI LVGL par app (nestor_ui, rappels_ui...)
```

---

## 3. Compagnon OS — Kernel FreeRTOS dual-core

### 3.1 Mapping des cores

| Core | Tâches |
|------|--------|
| Core 0 | wake word (ESP-SR), capture audio I2S, TTS playback, BLE stack, WiFi stack, timers réseau |
| Core 1 | LVGL render, input touch/boutons, OS main tick, agent brain ReAct, orchestrateur apps, schedulers |

### 3.2 Tâches FreeRTOS

```
task_ui_lvgl        (Core 1, priorité HAUTE)
  → lv_timer_handler() + traitement touch/boutons
  → 5ms tick typique

task_os_main        (Core 1, priorité NORMALE)
  → hal_pmu_tick(), wifi_mgr_tick(), ble_mgr_tick()
  → status_bar_tick(), agent_scheduler_tick()
  → app_orchestrator_tick() : active/suspend les apps
  → reminders_scheduler_tick() : vérifie les rappels à venir

task_voice_io       (Core 0, priorité HAUTE)
  → capture I2S mic → détection wake word ESP-SR
  → si wake word : wake from light sleep → lancer STT
  → lecture I2S DAC pour TTS output
  → buffers DMA, communication STT/TTS via file/semaphore

task_ble            (Core 0, priorité NORMALE)
  → GATT server : GPS, WIFI_SCAN, WIFI_PROVISION, AGENT_SYNC,
    TEXT_INPUT, LLM_RELAY, DEVICE_STATUS, REMINDERS_SYNC
  → gestion connect/disconnect, notifications

task_network        (Core 0, priorité BASSE)
  → WiFi connect/reconnect automatique
  → NTP sync → RTC sync
  → OTA check (si configuré)

task_agent_brain    (Core 1, priorité BASSE)
  → ReAct loop : plan → tool_call → observe → réponse
  → lecture/écriture mémoire L0-L4
  → appels LLM HTTP Groq (WiFi) ou via BLE LLM_RELAY
  → signale résultat à l'app active via event/queue
```

### 3.3 Gestion RAM des apps

- Les apps implémentent l'interface `app_base.h` :
  ```cpp
  class AppBase {
    virtual void init() = 0;   // une seule fois au boot (registration)
    virtual void start() = 0;  // create LVGL UI, alloc buffers, start tasks
    virtual void stop() = 0;   // delete LVGL screens, free all dynamic memory
    virtual void tick() {}     // appelé périodiquement si active
  };
  ```
- Quand une app n'est pas lancée : seul le registre + une icône launcher en mémoire.
- Le launcher + la status bar restent toujours en RAM.
- Aucune app non active n'alloue de RAM dynamique.

### 3.4 Light sleep et réveil

- En inactivité (configurable, ex. 5 min) : entrer en light sleep.
- Sources de réveil :
  - Wake word (audio continu sur Core 0 même en light sleep via ULP/I2S).
  - Timer RTC pour rappels planifiés.
  - Bouton physique (GPIO wakeup).
  - Touch screen (si configuré).
- À la sortie de light sleep : restaurer l'état de l'écran et de la tâche active.

---

## 4. Réseau : WiFi, BLE et fallback

### 4.1 WiFi — provisioning via PWA

- **Chemin principal** : provisioning depuis la PWA :
  1. L'ESP32 scanne les réseaux → résultats exposés via `WIFI_SCAN` BLE.
  2. Dans la PWA (vue Compagnon → section WiFi) : liste des réseaux, clic sur un réseau → modal saisie mdp.
  3. La PWA envoie `{ssid, password}` via `WIFI_PROVISION` BLE.
  4. L'ESP32 se connecte et persiste en NVS.
- **Fallback last resort** : portail AP `Compagnon_Setup` si jamais aucune connexion n'a pu être établie (ex. premier boot sans PWA disponible), timeout 180s.
- Niveaux de connectivité WiFi :
  - **FULL** : LLM, STT, TTS, APIs (météo, bourse, radars), sync PWA, OTA.
  - **PARTIAL** : seulement APIs critiques (météo, NTP, rappels).
  - **NO WIFI** : fallback BLE relay ou mode offline.

### 4.2 BLE — service Compagnon

UUID service principal : `4FAFC201-1FB5-459E-8FCC-C5C9C331914B`

| Caractéristique | UUID court | Dir. | Rôle |
|----------------|------------|------|------|
| GPS | `beb5483e-...` | N (notify) | lat,lon GPS ou fallback BLE |
| WIFI_SCAN | `...` | W+N | ESP32 scanne → notifie JSON résultats |
| WIFI_PROVISION | `...` | W | PWA envoie {ssid, pwd} → ESP32 connecte + NVS |
| AGENT_SYNC | `...` | W+N | échange JSON agents/skills/mémoire |
| TEXT_INPUT | `...` | W | PWA envoie texte → app courante |
| LLM_RELAY | `...` | W+N | requêtes LLM bidirectionnelles (ESP32 ↔ PWA) |
| DEVICE_STATUS | `...` | N | batterie %, mode WiFi/BLE, silencieux, heure |
| REMINDERS_SYNC | `...` | W+N | CRUD rappels bidirectionnel |

### 4.3 Fallback HTTP over BLE

- Si NO WIFI : les requêtes HTTP LLM/STT/TTS sont sérialisées en JSON sur `LLM_RELAY`.
- La PWA agit comme proxy : reçoit la requête, appelle Groq/Gemini, renvoie la réponse.
- Transparent pour l'agent brain (abstrait derrière `http_client`).

---

## 5. UI LVGL — Launcher + Status bar

### 5.1 Status bar (toujours visible)

- Dimensions : 480×36 px (Waveshare AMOLED 2.16", 480×480).
- Contenu de gauche à droite :
  - **Date et heure** format `"15 mai 2026 - 16:05"` (jj mmm aaaa - hh:mm, locale FR).
  - **Icône Bluetooth** : visible uniquement si un device BLE est appairé/connecté.
  - **Icône WiFi** : visible uniquement si connecté au WiFi.
  - **Jauge batterie** (widget LVGL) :
    - dessin d'une pile (contour + pôle positif à droite).
    - remplie proportionnellement au %.
    - **le % est affiché À L'INTÉRIEUR de la jauge** (label centré dessus).
    - couleur fill : > 30% → vert `#3a8a5a`, 15-30% → orange `#fa8030`, < 15% → rouge `#e84040`.

### 5.2 Launcher carousel

- Tileview LVGL avec 8 tiles (une par app).
- Chaque tile : icône LVGL, nom de l'app, badge de statut (ex. "3 rappels", "BTC +2.1%").
- Navigation :
  - Swipe gauche/droite (touch).
  - Boutons physiques LEFT/RIGHT.
  - Appui long RIGHT (ou touch sur la tile) → `app_start()` de l'app courante.
- Retour au launcher depuis une app : bouton HOME physique ou geste swipe down.

---

## 6. Voice : wake word, STT, TTS

### 6.1 Wake word (ESP-SR)

- Mot clé : "Nestor" (configurable dans `config/secrets.h`).
- Tourne en continu dans `task_voice_io` (Core 0), y compris depuis light sleep (via ULP ou I2S en low power).
- Au trigger :
  1. Exit light sleep si nécessaire.
  2. Écran allumé (sauf si mode silencieux total + config "écran off").
  3. Affichage vue "listening" LVGL (waveform animée).
  4. Capture audio X secondes.
  5. Envoi buffer audio → STT.

### 6.2 STT (Speech-to-Text)

- Priorité : STT Groq Whisper en HTTP direct (WiFi FULL).
- Fallback 1 : envoyer buffer audio compressé via BLE LLM_RELAY → PWA → Groq Whisper → renvoyer texte.
- Fallback 2 (dégradé) : commandes pré-définies locales si offline total.

### 6.3 Router vocal → apps

Texte STT → `voice_router` :
- Détection d'intention :
  - "ouvre [app]" → `app_start(app_name)`.
  - "rappelle-moi..." → agent Rappels.
  - "quel est le cours de..." → app Bourse.
  - sinon → app courante (ou app Nestor par défaut).
- Si une app est déjà ouverte, le texte lui est routé directement.

### 6.4 Bouton micro LVGL

- Dans les vues Nestor et Rappels : bouton mic (icône microphone, cliquable + tactile).
- Même pipeline que le wake word, sans mot clé : capture → STT → router → app.
- Utile quand le device est déjà allumé et actif.

### 6.5 TTS (Text-to-Speech)

- Cascade :
  1. Groq PlayAI TTS (HTTP, WiFi FULL).
  2. Fallback : TTS via BLE relay → PWA Web Speech API.
  3. Fallback offline : sons basiques I2S (bip/jingle) sans parole.
- Mode silencieux global :
  - Flag `silent_mode` en NVS.
  - Si ON : TTS désactivé, seulement UI visuelle.
  - Configurable depuis PWA (section config ESP32) et depuis Nestor ("mets-toi en silencieux").

---

## 7. App Rappels

### 7.1 Fonctionnalités

- Créer des rappels : depuis la PWA (formulaire ou input texte) ou par la voix (wake word / bouton micro).
- Configurer : titre/contenu, date, heure, avance avant rappel (0/5/15/30/60 min).
- À l'heure du rappel : ESP32 se réveille, joue un son + TTS du contenu (si pas silencieux).
- UI LVGL : affiche le rappel, propose "Fait" ou "Plus tard" (snooze +5/15 min).
- Historique des rappels passés (consultable dans l'app).

### 7.2 Modèle de données

Stockage : `FATFS:/reminders/reminders.json` (ou SD si disponible)

```json
{
  "reminders": [
    {
      "id": "uuid-v4",
      "title": "RDV dentiste",
      "body": "Dentiste rue de Rivoli — penser ordonnance",
      "datetime": "2026-05-20T09:00:00+02:00",
      "advance_minutes": 15,
      "repeat": null,
      "status": "scheduled",
      "created_at": "2026-05-15T16:00:00+02:00",
      "updated_at": "2026-05-15T16:00:00+02:00"
    }
  ]
}
```

Répétition future : champ `repeat` → `{type: "daily|weekly|monthly", days: [...], until: "date"}`.

### 7.3 Création depuis la PWA

- Vue Rappels : liste des rappels à venir + bouton "+ Nouveau rappel".
- Formulaire : titre, body (optionnel), date, heure, avance.
- OU : champ texte libre + parsing NLP côté PWA ("RDV dentiste demain 9h, rappel 15 min avant").
- Sync vers ESP32 via `REMINDERS_SYNC` BLE (commande `set_reminder`).

### 7.4 Création par la voix

1. Wake word → STT → texte "rappelle-moi de X le Y à Z".
2. Agent Rappels parse la phrase (date/heure/contenu).
3. Si infos manquantes → TTS "À quelle heure ?", écoute réponse.
4. Confirmation TTS : "D'accord, je te rappelle X le Y à Z."
5. Stockage + calcul prochaine alarme.

### 7.5 Réveil et notification

- `reminders_scheduler_tick()` vérifie chaque minute si un rappel est à déclencher.
- Si oui :
  1. Calcul `datetime - advance_minutes` → wakeup timer RTC.
  2. À l'heure : sortie light sleep, allumage écran.
  3. Son d'alarme I2S + TTS du contenu.
  4. Affichage UI : titre, body, heure, boutons "Fait" / "Snooze".

### 7.6 Sync bidirectionnelle

- Commandes `REMINDERS_SYNC` BLE :
  - `get_all` → ESP32 renvoie JSON liste complète.
  - `set_reminder {reminder}` → ESP32 persiste.
  - `delete_reminder {id}` → ESP32 supprime.
  - `reminder_event {id, status}` → notification de déclenchement/done.
- Résolution de conflits : `updated_at` timestamp, last-write-wins.

---

## 8. Agent Brain — Nestor (meilleur des deux repos)

### 8.1 Base : mimiclaw (Compagnon2)

Le brain embarqué côté firmware est basé sur l'architecture mimiclaw de Compagnon2, plus aboutie et optimisée que l'orchestrateur actuel de Compagnon v1. Points clés :
- **ReAct loop** : Reason → Act (tool call) → Observe → itère.
- **Tools** embarqués : get_weather, get_quote, http_get, set_reminder, get_reminders, control_ecovacs, smarthome_action...
- **Agents** définis en JSON dans FATFS (`/agents/default_agents.json`, `/agents/custom_agents.json`).

### 8.2 Mémoire hiérarchique L0-L4

| Niveau | Contenu | Stockage firmware | Stockage PWA |
|--------|---------|------------------|--------------|
| L0 | Contraintes système (langue, timezone, préférences vitales) | NVS + `/system/L0.json` | localStorage |
| L1 | Index de skills disponibles | `/system/skill_index.json` | IndexedDB |
| L2 | Faits utilisateur (MEMORY.md) | `/memory/MEMORY.md` | IndexedDB |
| L3 | Skills auto-cristallisées | `/skills/auto/*.json` | IndexedDB |
| L4 | Sessions passées (logs) | `/sessions/*.json` | IndexedDB |

### 8.3 Jardinier (consolidation mémoire)

- Tâche cron (hebdomadaire par défaut, ou sur commande vocale).
- Actions :
  - Supprime les skills peu utilisées (< 1 usage/semaine depuis 4 semaines).
  - Compacte les sessions en entrées MEMORY.md.
  - Ré-évalue les agents custom (redondants ?).
  - Génère un rapport bref → TTS + log dans MEMORY.md.
- Configurable depuis PWA (fréquence, seuils).

### 8.4 Fabrique (création d'agents)

- Commande vocale ou PWA : "Crée un agent qui [description]".
- Nestor génère le JSON d'un nouvel agent via LLM.
- L'agent est stocké dans `/agents/custom_agents.json`.
- Il est disponible immédiatement dans le router vocal et dans l'app Nestor.

### 8.5 Cristallisation automatique

- Quand un workflow est exécuté ≥ N fois avec les mêmes paramètres : l'orchestrateur crée automatiquement un skill en L3.
- Le skill est synchronisé vers la PWA via AGENT_SYNC.
- Côté PWA, même mécanique dans `orchestrator-engine.js`.

### 8.6 Tâches planifiées (cron)

- Dans `agent_scheduler` :
  - Jardinier (hebdo).
  - Refresh bourse (toutes les H en heures de marché).
  - Refresh météo (toutes les 3h).
  - Refresh radars (selon config).
  - Rappels (vérification chaque minute).
- Configurable depuis PWA (activer/désactiver chaque cron, changer la fréquence).

---

## 9. Stockage multi-niveaux

### 9.1 Firmware

| Niveau | Technologie | Contenu | Sans SD |
|--------|-------------|---------|----------|
| NVS | ESP32 NVS flash | WiFi creds, flags, préférences critiques | toujours dispo |
| FATFS flash | Partition 4-8MB | Agents, mémoire L0-L4, rappels, skills, sessions récentes | toujours dispo |
| SD card (optionnelle) | FAT32 sur microSD | Archives sessions longues, logs debug, assets audio/images | **fonctionnement sans SD** |

- Boot : tenter de monter la SD. Si échec → log warning, continuer en mode FATFS seul.
- Si SD absente : limiter l'historique des sessions (ex. 20 dernières seulement sur FATFS).
- Jamais de blocage, jamais d'erreur fatale si SD absente.

### 9.2 PWA

| Niveau | Technologie | Contenu |
|--------|-------------|---------||
| localStorage | Browser | Config, clés API, prefs UI |
| IndexedDB | Browser | Agents, mémoire, historique conversations, cache bourse/météo |
| Cache API | Service Worker | Assets PWA, réponses API récentes |

---

## 10. Compagnon PWA — Redesign responsive

### 10.1 Objectifs UX

- **Moins "copie LVGL"** : l'interface web doit être une vraie app mobile moderne, pas un miroir de l'écran ESP32.
- **Responsive** :
  - Breakpoint S : 375px (iPhone SE, iPhone 13 mini).
  - Breakpoint M : 390px (iPhone 13/14/15 — cible principale).
  - Breakpoint L : 768px (iPad, tablettes Android).
  - Breakpoint XL : 1024px+ (desktop/laptop pour gestion avancée).
- **Standalone PWA** : manifest, theme-color, display standalone, icon 512px, splashscreen.

### 10.2 Navigation

- **Bottom navigation bar** (mobile/tablette) : icônes des 8 apps + 1 bouton "Compagnon" (config ESP32).
- **Sidebar** (desktop) : même navigation en colonne à gauche.
- Header compact : logo Compagnon + indicateur statut ESP32 (connecté BLE ✓ / déconnecté).

### 10.3 Mode solo PWA (sans ESP32)

- Toutes les apps fonctionnent en mode cloud.
- Le statut BLE est "non connecté" : les sections de config ESP32 sont masquées ou grisées.
- Aucune fonctionnalité ne crash si l'ESP32 est absent.

### 10.4 Configuration ESP32 depuis PWA (vue Compagnon)

- Section **Connexion BLE** : bouton connecter/déconnecter, statut.
- Section **WiFi** : scan réseaux (via ESP32), sélection SSID, saisie mdp, envoi BLE.
- Section **Apps actives** : toggle par app (activer/désactiver sur le device).
- Section **Clés API** : saisie des clés (Groq, OpenWeatherMap, etc.) → push vers ESP32 via BLE.
- Section **Agents** : sync agents PWA ↔ ESP32.
- Section **Batterie/PMIC** : config + status batterie.
- Section **Rappels** : liste + CRUD + sync.
- Section **Préférences** : mode silencieux, luminosité, langue, timezone.

---

## 11. Sync bidirectionnelle PWA ↔ ESP32

### 11.1 Déclencheurs

- À la connexion BLE : sync automatique si `autoSync` activé.
- Manuellement : bouton "Synchroniser" dans la vue Compagnon.
- Sur événement : un rappel créé vocalement notifie la PWA via REMINDERS_SYNC.
- Sur cristallisation d'un agent : ESP32 notifie la PWA via AGENT_SYNC.

### 11.2 Protocole AGENT_SYNC

JSON message frames (déjà partiellement implémenté côté Compagnon v1) :
```json
{ "cmd": "get_agents", "ts": 1716000000 }
{ "cmd": "push_agents", "agents": [...], "ts": 1716000000 }
{ "cmd": "get_memory_l2", "ts": 1716000000 }
{ "cmd": "push_memory_l2", "data": "...", "ts": 1716000000 }
{ "cmd": "get_skills", "ts": 1716000000 }
{ "cmd": "push_skills", "skills": [...], "ts": 1716000000 }
{ "cmd": "get_config", "ts": 1716000000 }
{ "cmd": "push_config", "config": {...}, "ts": 1716000000 }
```

### 11.3 Résolution de conflits

- Règle principale : **last-write-wins** sur `updated_at` timestamp.
- Exception agents : fusion par ID (ne pas écraser si versions différentes → présenter dans PWA pour choix manuel).
- Jardinier peut arbitrer lors de sa prochaine passe.

---

## 12. Roadmap CompagnonV2

### Phase 1 — Firmware : base OS
1. Squelette PlatformIO ESP32-S3 + partition table + FATFS.
2. HAL : display (rm67162), touch, PMU (AXP2101), audio I2S.
3. Tâches FreeRTOS base : task_ui_lvgl + task_os_main + task_network + task_ble.
4. Status bar (format `jj mmm aaaa - hh:mm`, icônes BLE/WiFi, jauge batterie avec % dedans).
5. Launcher carousel 8 apps avec interface `AppBase`.

### Phase 2 — Firmware : apps existantes
6. Porter les 7 apps existantes (Bourse, Météo, Nestor, Musique, Radars, SmartHome, Ecovacs) dans la nouvelle architecture.
7. Implémenter BLE multi-caractéristiques complet (WIFI_SCAN, WIFI_PROVISION, AGENT_SYNC, TEXT_INPUT, LLM_RELAY, DEVICE_STATUS, REMINDERS_SYNC).

### Phase 3 — Firmware : voice + agents
8. Intégrer ESP-SR wake word + STT Groq + voice router.
9. Agent brain mimiclaw (ReAct, mémoire L0-L4, tools).
10. Jardinier + Fabrique + cristallisation auto.
11. App Rappels firmware (stockage, scheduler, TTS, UI LVGL).

### Phase 4 — PWA : redesign + app Rappels
12. Refactoriser PWA en architecture responsive (app-shell, bottom nav, breakpoints).
13. Ajouter vue Rappels PWA + sync BLE.
14. Améliorer vue Compagnon (config complète apps, clés API, prefs).

### Phase 5 — Sync + consolidation
15. Sync bidirectionnelle complète (agents, mémoire, rappels, config).
16. Tâches planifiées PWA (cron côté navigateur, ServiceWorker ou background sync).
17. Tests d'intégration PWA + firmware (scénarios end-to-end : wake word → rappel → sync PWA).
