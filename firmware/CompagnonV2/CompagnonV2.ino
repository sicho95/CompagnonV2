/*
 * CompagnonV2 — ESP32-S3 Waveshare AMOLED 2.16"
 * Framework : Arduino 3.3.8 / arduino-esp32
 * LVGL      : 9.x  (lv_init() OBLIGATOIRE avant tout appel LVGL)
 * Board     : Waveshare ESP32-S3-Touch-AMOLED-2.16
 *
 * Ordre d'init critique :
 *  0. lv_init()          — une seule fois, avant tout LVGL
 *  1. PMU                — rails ALDO1/ALDO3, bouton power
 *  2. NVS                — namespace "compagnon"
 *  3. secrets_provision  — charge secrets.h dans NVS si clés vides
 *  4. Display            — lv_display_create() + buffers PSRAM
 *  5. Touch              — 500 ms post-reset, CST9220
 *  6. IMU                — QMI8658
 *  7. Audio              — I2S mic + codec
 *  8. apps_register_all  — descripteurs statiques (RAM ~0)
 *  9. os::os_start()     — lance toutes les tâches FreeRTOS
 *                          (ui_lvgl, os_main, voice_io, ble, network)
 * 10. Callback PMU power → menu UI
 *
 * NOTE LVGL tick :
 *  LV_TICK_CUSTOM 1 dans lv_conf.h → LVGL lit millis() directement.
 *  Ne PAS appeler lv_tick_inc() — double-comptage qui bloque le rendu.
 *  lv_timer_handler() est appelé dans task_ui_lvgl (FreeRTOS), pas ici.
 */

#include <lvgl.h>

#include "src/hal/display.h"
#include "src/hal/touch.h"
#include "src/hal/imu.h"
#include "src/hal/pmu.h"
#include "src/hal/audio.h"
#include "src/system/os_main.h"
#include "src/system/os_kernel.h"
#include "src/system/power_mgr.h"
#include "src/config/nvs_config.h"
#include "src/storage/nvs_store.h"
#include "src/ui/status_bar.h"
#include <ArduinoJson.h>
#include <WiFi.h>

// secrets.h est dans include/ et dans .gitignore
// Copier depuis src/config/secrets_template.h et remplir les valeurs
#if __has_include("secrets.h")
  #include "secrets.h"
  #define HAS_SECRETS
#endif

// ─── Provisionnement NVS au premier boot ─────────────────────────────────────────────
static void secrets_provision() {
#ifndef HAS_SECRETS
    Serial.println("[BOOT] secrets.h absent — provisionnement ignoré");
    return;
#else
    Serial.println("[BOOT] Provisionnement NVS depuis secrets.h...");
    int written = 0;

    auto provision = [&](const char* key, const char* val) {
        if (!val || val[0] == '\0') return;
        char existing[128] = {};
        if (nvs_get_api_key(key, existing, sizeof(existing))) return;
        nvs_set_api_key(key, val);
        Serial.printf("  [NVS] %s → écrit\n", key);
        written++;
    };

    provision(NVS_KEY_GROQ,        SECRET_GROQ_API_KEY);
    provision(NVS_KEY_GEMINI,      SECRET_GEMINI_KEY);
    provision(NVS_KEY_SERPER,      SECRET_SERPER_KEY);
    provision(NVS_KEY_OPENROUTER,  SECRET_OPENROUTER_KEY);
    provision(NVS_KEY_TWELVEDATA,  SECRET_TWELVE_DATA_KEY);
#if defined(DEV_METEO_CONCEPT_TOKEN)
    provision(NVS_KEY_METEO,       DEV_METEO_CONCEPT_TOKEN);
#elif defined(SECRET_WEATHER_API_KEY)
    provision(NVS_KEY_METEO,       SECRET_WEATHER_API_KEY);
#endif
    provision(NVS_KEY_SPOTIFY_ID,  SECRET_SPOTIFY_CLIENT_ID);
    provision(NVS_KEY_SPOTIFY_SEC, SECRET_SPOTIFY_CLIENT_SEC);
    provision(NVS_KEY_TUYA_ID,     SECRET_TUYA_ACCESS_ID);
    provision(NVS_KEY_TUYA_SEC,    SECRET_TUYA_ACCESS_KEY);
    provision(NVS_KEY_ECOVACS_U,   SECRET_ECOVACS_ACCOUNT);
    provision(NVS_KEY_ECOVACS_P,   SECRET_ECOVACS_PASSWORD);

    {
        String existing = NvsStore::getString("wifi", "ssid", "");
        if (existing.isEmpty()) {
#if defined(SECRET_WIFI_SSID) && defined(SECRET_WIFI_PASS)
            NvsStore::setString("wifi", "ssid", SECRET_WIFI_SSID);
            NvsStore::setString("wifi", "pass", SECRET_WIFI_PASS);
            Serial.println("  [NVS] wifi → écrit");
            written++;
#endif
        }
    }

    if (written == 0)
        Serial.println("[BOOT] NVS déjà provisionné — rien écrit");
    else
        Serial.printf("[BOOT] Provisionnement terminé (%d clés écrites)\n", written);
#endif
}

// ─── Setup ──────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[BOOT] CompagnonV2 — démarrage");

    // 0. LVGL — une seule fois, avant tout appel LVGL
    lv_init();
    Serial.println("[BOOT] LVGL init OK");

    // 1-2. Hardware + NVS
    hal_pmu_init();
    nvs_config_init();

    // 3. Provisionnement NVS (secrets.h → NVS si clés vides)
    secrets_provision();

    // 4-7. Hardware suite
    hal::display_init();   // co5300::init() + lv_display_create() + buffers
    hal_touch_init();
    hal_imu_init();
    hal_audio_init();

    // 8. Enregistrer les apps dans le kernel (coût RAM ~0, pas de tâche)
    os::apps_register_all();

    // 9. Lancer toutes les tâches FreeRTOS
    os::os_start();

    // 10. Callback long-press PMU → menu power
    hal_pmu_set_long_press_cb(ui_power_menu_show);

    Serial.println("[BOOT] Prêt.");
}

// ─── Loop (Core 1) ──────────────────────────────────────────────────────────────
void loop() {
    hal_pmu_tick();
    power_mgr_tick();
    delay(20);
}
