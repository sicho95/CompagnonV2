# Waveshare ESP32-S3 Touch AMOLED 2.16 — Notes de bring-up

Ce document capitalise sur le travail de mise au point réalisé dans ce firmware pour la carte Waveshare `ESP32-S3-Touch-AMOLED-2.16` avec:

- Arduino IDE
- `arduino-esp32` `3.3.8`
- LVGL `9.x`
- écran AMOLED CO5300 en QSPI
- touch CST92xx via `SensorLib`

L'objectif est de garder une base réutilisable pour les prochains projets sur cette carte.

## Etat validé

Les points suivants fonctionnent ensemble dans ce projet:

- affichage LVGL sur la zone utile de l'écran
- orientation écran correcte avec `rotation=3` côté CO5300
- status bar LVGL stable
- launcher paginé avec navigation boutons
- fermeture d'app et retour launcher
- tactile fonctionnel dans LVGL

Fichiers de référence:

- [src/hal/display.cpp](/Users/damien/Documents/Arduino/CompagnonV2/src/hal/display.cpp)
- [src/hal/touch.cpp](/Users/damien/Documents/Arduino/CompagnonV2/src/hal/touch.cpp)
- [src/system/os_main.cpp](/Users/damien/Documents/Arduino/CompagnonV2/src/system/os_main.cpp)
- [src/ui/launcher.cpp](/Users/damien/Documents/Arduino/CompagnonV2/src/ui/launcher.cpp)
- [include/pins.h](/Users/damien/Documents/Arduino/CompagnonV2/include/pins.h)

## Géométrie écran

La carte annonce un écran physique `480x480`, mais la zone réellement visible est masquée par le boîtier.

Configuration retenue:

- physique: `480x480`
- marges boîtier: `20 px` gauche/droite, `10 px` haut/bas
- zone utile LVGL: `440x460`

Définition utilisée:

- `LCD_WIDTH_PHYS = 480`
- `LCD_HEIGHT_PHYS = 480`
- `LCD_MARGIN_H = 20`
- `LCD_MARGIN_V = 10`
- `LCD_WIDTH = 440`
- `LCD_HEIGHT = 460`

Conséquence:

- le driver écran est initialisé en `480x480`
- LVGL est enregistré en `440x460`
- le flush ajoute l'offset boîtier avant envoi au CO5300

Le flush qui marche est dans [src/hal/display.cpp](/Users/damien/Documents/Arduino/CompagnonV2/src/hal/display.cpp):

```cpp
co5300::flush(
    area->x1 + LCD_MARGIN_H,
    area->y1 + LCD_MARGIN_V,
    area->x2 + LCD_MARGIN_H,
    area->y2 + LCD_MARGIN_V,
    (const uint16_t*)px_map
);
```

## Orientation écran

Réglage retenu:

- `LCD_ROTATION = 3`

Ce réglage est cohérent avec le CO5300 de cette carte et le rendu LVGL actuellement validé.

Quand l'affichage paraissait décalé de `+90°`, le vrai problème n'était pas uniquement LVGL. Il fallait garder:

- la rotation matérielle CO5300 qui donne le bon rendu visible
- la transformation tactile cohérente avec cette orientation

## LVGL: point essentiel pour le tactile

Le tactile ne générait rien tant que l'indev LVGL n'était pas explicitement rattaché au display.

Point bloquant résolu dans [src/system/os_main.cpp](/Users/damien/Documents/Arduino/CompagnonV2/src/system/os_main.cpp):

```cpp
lv_indev_t* touch_indev = lv_indev_create();
lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
lv_indev_set_read_cb(touch_indev, _touch_read_cb);
lv_indev_set_display(touch_indev, lv_display_get_default());
```

Sans `lv_indev_set_display(...)`, le touch pouvait être créé mais aucun événement utile n'arrivait dans l'UI.

## Règle de thread LVGL

Une seule tâche appelle LVGL: la tâche UI.

Dans ce projet:

- `task_ui_lvgl()` est la seule tâche à appeler `lv_timer_handler`, `lv_scr_load`, `lv_refr_now`, etc.
- les autres tâches postent ou mettent à jour leur état, mais ne pilotent pas directement LVGL

Cette règle évite des comportements incohérents au retour launcher / ouverture d'app.

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
3. transformation locale des coordonnées vers la zone utile LVGL

Code retenu dans [src/hal/touch.cpp](/Users/damien/Documents/Arduino/CompagnonV2/src/hal/touch.cpp):

```cpp
_drv.setMaxCoordinates(480, 480);
_drv.setSwapXY(true);
_drv.setMirrorXY(true, false);
```

et pour la lecture:

```cpp
const TouchPoints& points = _drv.getTouchPoints();
uint8_t count = points.getPointCount();
if (count > 0) {
    const TouchPoint& pt = points.getPoint(0);
    _map_touch_to_lvgl(pt.x, pt.y, x, y);
    return true;
}
```

### Transformation tactile retenue

Le touch renvoie des coordonnées sur `480x480`. Ensuite on retire les marges du boîtier:

```cpp
int32_t lx = sensor_x - LCD_MARGIN_H;
int32_t ly = sensor_y - LCD_MARGIN_V;
```

puis on clamp vers la zone LVGL `440x460`.

Cette combinaison fonctionne avec:

- `setSwapXY(true)`
- `setMirrorXY(true, false)`
- `rotation=3` côté écran
- les marges visibles `20 px / 10 px` retranchées après lecture capteur

Ce réglage est aligné sur l'exemple Waveshare LVGL pour cette carte. Le symptôme typique d'un miroir faux était simple:

- le touch semblait "vivre"
- les logs montraient des coordonnées plausibles
- mais le clic tombait sur la mauvaise icône ou sur la mauvaise app

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

Ce log apparaissait avec une ancienne variante. La lecture actuelle via `getTouchPoints()` puis `getPoint(0)` uniquement quand `count > 0` évite ce faux accès.

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

1. Garder l'écran en `480x480` physique mais enregistrer LVGL en zone utile `440x460`.
2. Appliquer les offsets de marges dans le `flush`, pas en trichant dans toutes les apps.
3. Rattacher explicitement le tactile au display LVGL avec `lv_indev_set_display(...)`.
4. Ne pas bricoler une lecture raw I2C du CST92xx si `SensorLib` est déjà capable de parler au contrôleur.
5. Ne pas réinitialiser `Wire` dans le module touch.
6. Garder une seule tâche propriétaire de LVGL.
7. Faire en sorte que la zone tactile d'une icône soit plus grande que son dessin.
8. Si le tactile lance la mauvaise app, vérifier d'abord `swap/mirror` avant de toucher à la logique du launcher.

## Ce qu'il reste à faire plus tard

- rendre le fond du launcher configurable via image stockée en FATFS/FFat
- brancher cette config via la PWA
- finaliser l'app Réglages
- harmoniser complètement `ui_config.h` et les dimensions réellement utilisées par la status bar / launcher
