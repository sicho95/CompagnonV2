# Waveshare ESP32-S3 Touch AMOLED 2.16 — Notes de bring-up

Ce document capitalise sur le travail de mise au point réalisé dans ce firmware pour la carte Waveshare `ESP32-S3-Touch-AMOLED-2.16` avec:

- Arduino IDE
- `arduino-esp32` `3.3.8`
- LVGL `9.x`
- écran AMOLED CO5300 en QSPI
- touch CST92xx via `SensorLib`

L'objectif est de garder une base réutilisable pour les prochains projets sur cette carte.

## Synthèse validée juin 2026

Le point critique de cette carte est que le CO5300 via `Arduino_GFX` ne fait pas une vraie rotation 90/270. `Arduino_CO5300::setRotation()` ne fait que des flips MADCTL. Il ne faut donc pas compter sur la rotation matérielle pour remettre l'UI droite.

Architecture validée:

- écran CO5300 initialisé en rotation matérielle `0`
- LVGL déclaré en plein `480x480`
- contenu applicatif contraint à une safe area `450x460` dans le layout
- `LCD_ROTATION = 3` conservé comme orientation de montage historique
- orientation de montage convertie en rotation logicielle inverse dans `display.cpp`
- `flush_cb` tourne les pixels avant `draw16bitRGBBitmap()`
- CST9220 lu brut via `SensorLib`, sans `swapXY` ni `mirrorXY`
- `touch.cpp` mappe `raw_x/raw_y` vers `x/y` dans le repère LVGL visible selon `display_get_rotation()`
- LVGL reste en rotation interne `0` pour éviter une seconde transformation automatique
- l'indev LVGL est rattaché au display, son timer interne est mis en pause, puis lu explicitement une seule fois par boucle UI
- `app_launch()` et `app_close_current()` passent toujours par une transition atomique sur le thread UI
- quand `launch/close` part d'un callback LVGL, la transition est différée via `lv_async_call()`
- aucune I/O réseau bloquante d'application ne s'exécute dans `init()` ou `onResume()` sur le thread UI
- QMI8658 ajoute une rotation UI dynamique, avec offset de `180°` entre son repère et le repère visuel du boîtier
- `display_set_rotation()` invalide l'écran actif et la couche top, puis force `lv_refr_now()`

Logs attendus au boot validé:

```text
[CO5300] init OK — hw_rotation=0 (software rotation=3)
[HAL] LVGL init OK — mount_rotation=1 ui_rotation=0 flush_rotation=1
[TOUCH] CST9220 OK — CST9220 (RST=40 INT=11 irq=1)
[TOUCH] LVGL probe armed
```

Logs attendus lors d'un tap valide:

```text
[TOUCH] sensor=138,256 count=1 -> lv=223,138 int=0
[TOUCH] x=223 y=138 (raw=138,256)
[UI] launcher sel page=0 slot=1 idx=1
[UI] launch app 4 (touch)
```

## Etat validé

Les points suivants fonctionnent ensemble dans ce projet:

- affichage LVGL plein écran `480x480`
- orientation écran correcte au boot via rotation logicielle de flush
- rotation auto QMI8658 dans le bon sens
- status bar LVGL stable
- launcher paginé avec navigation boutons
- fermeture d'app et retour launcher
- tactile fonctionnel dans LVGL et aligné avec la rotation affichée

Fichiers de référence:

- [src/hal/display.cpp](/Users/damien/Documents/Arduino/CompagnonV2/src/hal/display.cpp)
- [src/hal/touch.cpp](/Users/damien/Documents/Arduino/CompagnonV2/src/hal/touch.cpp)
- [src/hal/imu.cpp](/Users/damien/Documents/Arduino/CompagnonV2/src/hal/imu.cpp)
- [src/system/os_main.cpp](/Users/damien/Documents/Arduino/CompagnonV2/src/system/os_main.cpp)
- [src/ui/launcher.cpp](/Users/damien/Documents/Arduino/CompagnonV2/src/ui/launcher.cpp)
- [include/pins.h](/Users/damien/Documents/Arduino/CompagnonV2/include/pins.h)

## Géométrie écran

La carte annonce un écran physique `480x480`. Pour garder un repère identique entre dalle, touch et LVGL, le display LVGL est aussi déclaré en `480x480`.

Configuration retenue:

- physique: `480x480`
- logique LVGL: `480x480`
- marges boîtier: disponibles comme safe area layout seulement

Définition utilisée:

- `LCD_WIDTH_PHYS = 480`
- `LCD_HEIGHT_PHYS = 480`
- `LCD_MARGIN_H = 15`
- `LCD_MARGIN_V = 10`
- `LCD_WIDTH = LCD_WIDTH_PHYS`
- `LCD_HEIGHT = LCD_HEIGHT_PHYS`
- `LCD_SAFE_WIDTH = 450`
- `LCD_SAFE_HEIGHT = 460`

Conséquence:

- le driver écran est initialisé en `480x480`
- LVGL est enregistré en `480x480`
- le touch CST9220 reste dans le même repère brut `480x480`
- les marges ne doivent pas être soustraites dans le HAL tactile
- le masque boîtier est traité uniquement comme contrainte de layout via `LCD_SAFE_X/Y/W/H`

La zone utile éventuelle doit être traitée par le layout applicatif, pas en modifiant les coordonnées du display ou du touch.

La safe area retenue sur ce boîtier est:

- `x = 15 .. 464`
- `y = 10 .. 469`
- `w = 450`
- `h = 460`

Concrètement:

- status bar, launcher, footer dots, notifications et bouton `X` restent dans cette zone
- les écrans d'app restent en fond `480x480`, mais leurs widgets utiles se placent dans `ui_app_content_x()` / `ui_app_content_width()`

## Orientation écran

Réglages validés:

- `LCD_ROTATION = 3` dans `include/pins.h`
- `co5300::init()` appelle `_gfx->setRotation(0)`
- `display.cpp` convertit `LCD_ROTATION` en rotation logicielle de montage inverse:

```cpp
return (lv_display_rotation_t)((4 - (LCD_ROTATION & 0x3)) & 0x3);
```

Avec `LCD_ROTATION = 3`, le log validé indique:

```text
mount_rotation=1 ui_rotation=0 flush_rotation=1
```

La rotation effective utilisée par le flush est:

```cpp
flush_rotation = mount_rotation + ui_rotation mod 4
```

`ui_rotation` reste `0` au boot et devient dynamique avec le QMI8658.

Point important: ne pas remettre `_gfx->setRotation(LCD_ROTATION)`. Sur CO5300 cela ne produit pas une vraie rotation et désynchronise le raisonnement touch/display.

## Rotation auto QMI8658

Le QMI8658 est lu dans `hal_imu_tick()` avec filtrage:

- seuil axe minimal: `0.45 g`
- dominance entre axes: `0.16 g`
- stabilité: `4` échantillons

La tâche UI interroge l'IMU toutes les `200 ms`. Quand l'orientation stable change, elle appelle:

```cpp
hal::display_set_rotation(_imu_orientation_to_rotation(ori));
```

Le repère du QMI8658 est à `180°` du repère visuel du boîtier. Table validée:

```cpp
orientation=0 -> ui_rotation=180
orientation=1 -> ui_rotation=90
orientation=2 -> ui_rotation=0
orientation=3 -> ui_rotation=270
```

Quand la rotation UI change, `display_set_rotation()` doit forcer un redraw:

```cpp
lv_obj_invalidate(lv_screen_active());
lv_obj_invalidate(lv_layer_top());
lv_refr_now(_disp);
```

## LVGL: point essentiel pour le tactile

Le tactile ne générait rien tant que l'indev LVGL n'était pas explicitement rattaché au display.

Point bloquant résolu dans [src/system/os_main.cpp](/Users/damien/Documents/Arduino/CompagnonV2/src/system/os_main.cpp):

```cpp
lv_indev_t* touch_indev = lv_indev_create();
lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
lv_indev_set_read_cb(touch_indev, _touch_read_cb);
lv_indev_set_display(touch_indev, lv_display_get_default());
lv_timer_pause(lv_indev_get_read_timer(touch_indev));
```

Sans `lv_indev_set_display(...)`, le touch pouvait être créé mais aucun événement utile n'arrivait dans l'UI.

Dans ce projet, le timer interne de lecture LVGL est mis en pause. La tâche UI fait ensuite une lecture explicite unique par boucle:

```cpp
ui::dispatch_flush();
lv_indev_read(touch_indev);
lv_timer_handler();
ui_status_bar_touch_tick();
ui_launcher_touch_tick();
ui::notification_tick();
```

Ce point a été nécessaire sur cette stack:

- sans lecture explicite, plus aucun log tactile n'apparaissait
- avec deux lectures par boucle, les séquences `press/release/click` pouvaient être cassées
- si `ui_launcher_touch_tick()` passait avant `lv_timer_handler()`, certains `LV_EVENT_CLICKED` pouvaient être perturbés

## Règle de thread LVGL

Une seule tâche appelle LVGL: la tâche UI.

Dans ce projet:

- `task_ui_lvgl()` est la seule tâche à appeler `lv_timer_handler`, `lv_scr_load`, `lv_refr_now`, etc.
- les autres tâches postent ou mettent à jour leur état, mais ne pilotent pas directement LVGL

Cette règle évite des comportements incohérents au retour launcher / ouverture d'app.

Corollaire important pour ce projet:

- `app_launch()` et `app_close_current()` ne doivent pas modifier l'état écran hors thread UI
- si l'appel vient déjà d'un callback LVGL, la transition est reportée au prochain `lv_timer_handler()` via `lv_async_call()`
- sinon elle est postée via `ui::dispatch_post(...)`
- `_current_app` n'est changé qu'au moment de la transition UI effective
- un verrou court ignore les commandes `launch/close` concurrentes ou trop rapprochées pendant la stabilisation
- le cooldown de stabilisation s'applique aux `launch`; le bouton `X` peut fermer l'app dès qu'elle est visible
- les transitions `launch/close`, `onResume()` et callbacks de données ne doivent pas appeler `display_force_refresh()` / `lv_refr_now()`
- après un changement d'écran, on invalide les objets et on laisse le tick LVGL normal faire le redraw complet
- si le panel doit être rafraîchi immédiatement après une transition, appeler `display_request_refresh()`
- `task_ui_lvgl()` consomme cette demande au tour suivant, après `dispatch_flush()`, donc hors callback `lv_async_call()`

Cette règle évite le cas où une app serait marquée fermée logiquement, mais resterait visuellement affichée si la queue UI échouait.
Elle évite aussi le gel observé quand le launcher planifie sa finalisation puis qu'un refresh synchrone bloque la boucle UI avant l'exécution de cette finalisation.

Debug de gel UI:

- `UI_TRACE` dans `src/system/os_main.cpp` active un heartbeat `UI_LOOP`
- si le dernier log est `UI_REFRESH begin` sans `end`, le blocage est dans `lv_refr_now()` / flush CO5300
- si le dernier log est `lv_timer slow` ou aucun heartbeat ensuite, le blocage est côté LVGL/timer
- `dispatch=N` montre combien de callbacks UI ont été exécutés dans le tour courant
- le retour launcher ne doit pas demander de `display_request_refresh()`: le log a montré un blocage dans `lv_refr_now()` après fermeture d'app

Collision de zones UI:

- la zone du bouton `X` des apps ne doit pas se superposer à une tuile cliquable du launcher
- le launcher réserve donc une bande haute non interactive et décale sa grille sous cette zone
- `ui_launcher_show()` bloque aussi les taps pendant ~250 ms pour absorber le relâchement du doigt après fermeture d'app

Autre règle critique:

- `init()` et `onResume()` ne doivent pas faire d'HTTP bloquant, DNS, TLS, ni de lecture lente
- si une app doit charger des données réseau, elle le fait dans une tâche de fond
- le résultat revient ensuite sur LVGL via `ui::dispatch_post(...)`

Exemple validé dans ce projet:

- `Meteo` ne fait plus son `HTTPClient::GET()` dans `onResume()`
- le fetch tourne dans une tâche FreeRTOS dédiée avec timeout explicite
- l'UI reste donc réactive: rotation IMU, bouton `X`, boutons physiques, retour launcher

## I2C partagé: ce qu'il faut faire

Le bus I2C de cette carte est partagé entre:

- touch
- PMU AXP2101
- IMU QMI8658
- RTC
- codecs audio

Brochage:

- `SCL = GPIO14`
- `SDA = GPIO15`

Point important:

- le bus I2C est déjà monté ailleurs au boot
- le touch ne doit pas reconfigurer `Wire` comme s'il possédait seul le bus

Approche retenue:

- laisser l'initialisation I2C au HAL déjà en place
- appeler `SensorLib` avec `Wire` déjà prêt
- ne pas refaire `Wire.setPins(...)`
- ne pas refaire `Wire.begin(...)` pour le touch

Dans [src/hal/touch.cpp](/Users/damien/Documents/Arduino/CompagnonV2/src/hal/touch.cpp):

```cpp
_drv.setPins(PIN_TP_RST, PIN_TP_INT);
if (_drv.begin(Wire, CST92XX_SLAVE_ADDRESS, -1, -1)) {
```

Les warnings `bus already initialized` peuvent encore apparaître à cause de bibliothèques tierces, mais ce n'est pas en soi le problème bloquant.

## Tactile: ce qui a réellement débloqué la situation

### Ce qui ne marchait pas

Une tentative intermédiaire lisait le CST92xx en brut via `Wire` avec:

- commande `0xD000`
- ACK manuel `0xAB`
- séquence read/write artisanale

Cette approche provoquait des rafales de:

- `ESP_ERR_INVALID_STATE`
- `raw i2c read failed`

Conclusion:

- sur cette stack Arduino + ESP32 actuelle, cette lecture I2C manuelle n'était pas fiable
- elle cassait le flux tactile au lieu de le débloquer

### Ce qui marche

Le tactile fonctionne en revenant à une chaîne simple:

1. initialisation du CST92xx via `SensorLib`
2. lecture du point via `SensorLib`
3. mapping local des coordonnées vers le repère LVGL visible

Code retenu dans [src/hal/touch.cpp](/Users/damien/Documents/Arduino/CompagnonV2/src/hal/touch.cpp):

```cpp
_drv.setMaxCoordinates(480, 480);
_drv.setSwapXY(false);
_drv.setMirrorXY(false, false);
```

et pour la lecture:

```cpp
uint8_t count = _drv.getPoint(_raw_x, _raw_y, max_points);
```

### Transformation tactile retenue

Le touch renvoie des coordonnées physiques `480x480`. `raw_x/raw_y` sont conservés pour debug, puis `x/y` sont mappés dans le repère LVGL visible selon `hal::display_get_rotation()`.

```cpp
ROT_0   : x = raw_x,             y = raw_y
ROT_90  : x = (LCD_HEIGHT-1)-raw_y, y = raw_x
ROT_180 : x = (LCD_WIDTH-1)-raw_x,  y = (LCD_HEIGHT-1)-raw_y
ROT_270 : x = raw_y,                y = (LCD_WIDTH-1)-raw_x
```

Cette combinaison fonctionne avec:

- `setSwapXY(false)`
- `setMirrorXY(false, false)`
- LVGL déclaré en `480x480`
- rotation affichage faite dans le flush
- mapping tactile fait dans `touch.cpp`

Le symptôme typique d'un mapping faux:

- le touch semblait "vivre"
- les logs montraient des coordonnées plausibles
- mais le clic tombait sur la mauvaise icône ou sur la mauvaise app

Le symptôme typique d'une lecture indev mal orchestrée:

- le touch est initialisé
- mais aucun `[TOUCH] sensor=...` n'apparaît après le boot
- ou les coordonnées apparaissent mais `LV_EVENT_CLICKED` ne déclenche pas

Le correctif validé est: un seul `lv_indev_read(touch_indev)` explicite par boucle UI, avec le timer interne en pause.

## Touch IRQ

Configuration retenue:

- `TP_INT = GPIO11`
- `TP_RST = GPIO40`

Le touch est piloté en mode mixte:

- flag d'interruption via ISR
- fallback polling léger
- maintien bref de la dernière coordonnée pour stabiliser le pointer LVGL

Extrait utile:

```cpp
attachInterrupt(digitalPinToInterrupt(PIN_TP_INT), _touch_irq_isr, FALLING);
```

et dans `touch_read()`:

- lecture si IRQ vue
- ou si niveau IRQ bas
- ou si polling périodique arrivé

## Boutons physiques

Brochage retenu:

- `KEY3 = GPIO18`
- `BOOT = GPIO0`

Sémantique actuelle dans [src/ui/launcher.cpp](/Users/damien/Documents/Arduino/CompagnonV2/src/ui/launcher.cpp):

- `KEY3 court` -> sélection suivante
- `BOOT court` -> sélection précédente
- `KEY3 long` -> lancer l'app sélectionnée
- `BOOT long` -> fermer l'app courante

Point important:

- `BOOT long` reste actif même hors launcher
- la navigation courte, elle, n'agit que lorsque le launcher est l'écran actif

## Launcher LVGL validé

Le launcher actuellement validé:

- fond noir AMOLED
- grille `3 colonnes x 2 lignes`
- `6` icônes par page
- page dots
- zone tactile des icônes plus large que le simple pictogramme

La cible tactile ne doit pas être limitée au disque de l'icône. Toute la cellule doit être cliquable.

## Symptômes déjà vus et leur interprétation

### `LVGL probe armed` mais aucun touch

Ca voulait simplement dire:

- l'indev LVGL existe
- pas que les événements remontent correctement

### `ESP_ERR_INVALID_STATE` en boucle sur `i2c_master_transmit`

Ca indiquait un mauvais chemin de lecture tactile bas niveau. Le correctif utile a été:

- suppression de la lecture raw I2C maison
- retour à `SensorLib`

### `Invalid touch point index: 0`

Ce log apparaît si on demande un point alors que l'objet interne n'a pas de point valide au moment précis de l'accès.

Ce log peut encore apparaître ponctuellement dans `TouchPoints.cpp` lors du relâchement, mais il n'est pas bloquant si les logs suivants existent:

```text
[TOUCH] sensor=... -> lv=...
[UI] launcher sel ...
[UI] launch app ...
```

Il devient bloquant uniquement s'il s'accompagne d'une absence de coordonnées ou d'une absence d'événements launcher.

## Réglages de build validés

Compilation testée avec:

- board: `ESP32S3 Dev Module`
- flash: `16MB`
- PSRAM: `OPI`
- partition: `Custom`
- CPU: `240 MHz`
- USB mode: `Hardware CDC`

Commande de compilation utilisée côté CLI:

```sh
/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli compile \
  --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,PartitionScheme=custom,CPUFreq=240,USBMode=hwcdc \
  --libraries /Users/damien/Documents/Arduino/libraries \
  /Users/damien/Documents/Arduino/CompagnonV2
```

## Règles à réutiliser sur un autre projet

1. Garder écran et LVGL en `480x480`.
2. Ne pas utiliser `Arduino_CO5300::setRotation()` pour une vraie rotation 90/270.
3. Faire la rotation 90/270 dans le `flush_cb`, avec un buffer temporaire.
4. Garder `LCD_ROTATION` comme orientation de montage, mais inverser son sens pour la rotation logicielle.
5. Garder `raw_x/raw_y` CST9220 en debug et mapper `x/y` selon `display_get_rotation()`.
6. Ne pas appliquer `swapXY`/`mirrorXY` dans SensorLib tant que le mapping logiciel gère la rotation.
7. Rattacher explicitement le tactile au display LVGL avec `lv_indev_set_display(...)`.
8. Mettre en pause le timer interne indev et faire une seule lecture explicite par boucle UI.
9. Ne pas bricoler une lecture raw I2C du CST92xx si `SensorLib` est déjà capable de parler au contrôleur.
10. Ne pas réinitialiser `Wire` dans le module touch.
11. Garder une seule tâche propriétaire de LVGL.
12. Faire en sorte que la zone tactile d'une icône soit plus grande que son dessin.
13. Si le tactile lance la mauvaise app, vérifier d'abord la formule `display_get_rotation() -> touch map`.
14. Si le tactile ne log plus rien, vérifier d'abord la lecture indev LVGL.
15. Si le boot est droit mais la rotation auto est inversée de `180°`, corriger la table QMI8658 -> `ui_rotation`, pas le flush de base.
16. Ne jamais réduire LVGL à `450x460`: garder `480x480` et appliquer la safe area dans le layout.
17. Le bouton `X` d'une app doit être créé au premier plan, après le contenu principal.
18. Les transitions `launch/close` doivent rester atomiques sur le thread UI.
19. Aucune app ne doit faire d'I/O réseau bloquante dans `init()` ou `onResume()`.

## Ce qu'il reste à faire plus tard

- rendre le fond du launcher configurable via image stockée en FATFS/FFat
- brancher cette config via la PWA
- finaliser l'app Réglages
- harmoniser complètement `ui_config.h` et les dimensions réellement utilisées par la status bar / launcher
