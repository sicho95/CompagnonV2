# CompagnonV2 — Spécification fonctionnelle complète

> Version v2.7 — mai 2026  
> Fusion de Compagnon (PWA + firmware) et Compagnon2 (voice OS + agents mimiclaw)

---

## 0. Stack technique de référence

| Composant | Version choisie | Raison |
|-----------|----------------|--------|
| **Arduino ESP32** | **3.3.8** | Dernière stable, basée sur IDF 5.5.4 |
| **LVGL** | **9.x** | Version 9 supportée par Arduino ESP32 3.3.8, API moderne |
| **Build system** | **Arduino IDE / PlatformIO** | platformio.ini ou sketch selon workflow |
| **Arduino_GFX_Library** | dernière stable | Driver CO5300 QSPI (moononournation) |
| **SensorLib** | dernière stable | Touch **CST9220** + RTC PCF85063 + IMU QMI8658 |
| **XPowersLib** | **0.2.x** | Pilote officiel AXP2101 |
| **ArduinoJson** | **7.3.x** | API moderne, zéro copie |

## 1. Cartographie matérielle validée

Ces valeurs proviennent des schémas matériels fournis et doivent rester la référence unique pour éviter toute régression d’adressage ou de GPIO.

### 1.1 Bus I2C partagé

- SDA = GPIO15
- SCL = GPIO14
- Périphériques sur ce bus : CST9220, PCF85063, AXP2101, QMI8658, codecs audio de contrôle.

### 1.2 Écran CO5300 AMOLED (connecteur J4)

- LCD_CS = GPIO12
- QSPI_SCL = GPIO38
- QSPI_SI0 = GPIO4
- QSPI_SI1 = GPIO5
- QSPI_SI2 = GPIO6
- QSPI_SI3 = GPIO7
- LCD_RESET = GPIO39
- LCD_TE = GPIO8

### 1.3 Touch CST9220

- TP_INT = GPIO11
- TP_RESET = GPIO40
- TP_SDA = GPIO15
- TP_SCL = GPIO14
- Adresse I2C = 0x1A

### 1.4 RTC PCF85063

- RTC_INT = GPIO13
- RTC_SDA = GPIO15
- RTC_SCL = GPIO14
- Adresse I2C = 0x51

### 1.5 IMU QMI8658

- QMI_INT1 = GPIO17
- QMI_INT2 = GPIO21
- QMI_SDA = GPIO15
- QMI_SCL = GPIO14
- Adresse I2C = 0x6B

### 1.6 Audio / I2S

- ES8311 DSDIN = GPIO8
- ES8311 SCLK = GPIO9
- ES8311 ASDOUT = GPIO10
- I2S_MCLK = GPIO42
- I2S_LRCK = GPIO45
- PA_CTRL = GPIO46

### 1.7 SD / alimentation / USB

- SD_MOSI = GPIO1
- SD_SCK = GPIO2
- SD_MISO = GPIO3
- SD_CS = GPIO41
- SYS_OUT = GPIO16
- USB D- = GPIO19
- USB D+ = GPIO20
- UART TX = GPIO43
- UART RX = GPIO44

## 2. Règles firmware

- Toute définition de pins doit être centralisée dans `firmware/include/pins.h`.
- Aucun doublon de `pins.h` ne doit exister dans `firmware/src/config/`.
- Toute correction matérielle validée sur schéma doit être reportée dans cette spec en même temps que dans le firmware.
- Pour l’I2C, `Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL)` doit être effectué une seule fois côté PMU/HAL puis réutilisé partout.
- L’écran, le touch, le RTC et l’IMU doivent tous utiliser les GPIO ci-dessus sans hardcode local divergent.
