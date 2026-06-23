// ============================================================
// ble_manager.cpp — GATT Server CompagnonV2 (NimBLE)
//
// Caracteristiques :
//   WIFI_SCAN      2002  R/N  => liste reseaux JSON
//   WIFI_PROVISION 2003  W    => SSID+pass JSON => NVS
//   AGENT_SYNC     2004  R/W/N=> payload JSON bidirectionnel
//   TEXT_INPUT     2005  W    => texte libre => orchestrateur
//   LLM_RELAY      2006  R/W/N=> requete/reponse LLM sans WiFi
//   DEVICE_STATUS  2007  R/N  => JSON status
//   GPS            2008  R/N  => JSON lat/lon/alt
//
// fix: svc->start() deprecie dans NimBLE 2.x => supprime
//      Les services sont demarres automatiquement par _server->start()
// ============================================================
#include "ble_manager.h"
#include "wifi_mgr.h"
#include "../config/nvs_config.h"
#include "../storage/nvs_store.h"
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Arduino.h>

#define SVC_MAIN       "12345678-1234-1234-1234-1234567890ab"
#define CHR_WIFI_SCAN  "00002002-0000-1000-8000-00805f9b34fb"
#define CHR_WIFI_PROV  "00002003-0000-1000-8000-00805f9b34fb"
#define CHR_AGENT_SYNC "00002004-0000-1000-8000-00805f9b34fb"
#define CHR_TEXT_INPUT "00002005-0000-1000-8000-00805f9b34fb"
#define CHR_LLM_RELAY  "00002006-0000-1000-8000-00805f9b34fb"
#define CHR_DEV_STATUS "00002007-0000-1000-8000-00805f9b34fb"
#define CHR_GPS        "00002008-0000-1000-8000-00805f9b34fb"

namespace ble {

static TextInputCb  _on_text  = nullptr;
static AgentSyncCb  _on_agent = nullptr;
static LlmRelayCb   _on_llm   = nullptr;

static NimBLECharacteristic* _chr_wifi_scan  = nullptr;
static NimBLECharacteristic* _chr_wifi_prov  = nullptr;
static NimBLECharacteristic* _chr_agent_sync = nullptr;
static NimBLECharacteristic* _chr_text_input = nullptr;
static NimBLECharacteristic* _chr_llm_relay  = nullptr;
static NimBLECharacteristic* _chr_dev_status = nullptr;
static NimBLECharacteristic* _chr_gps        = nullptr;
static NimBLEServer*         _server         = nullptr;
static bool                  _connected      = false;
static String                _last_wifi_scan = "[]";

static void _notify_json(NimBLECharacteristic* chr, const String& json) {
    if (!chr) return;
    chr->setValue(json.c_str());
    if (_connected) chr->notify();
}

static bool _allowed_api_key(const String& key) {
    static const char* keys[] = {
        NVS_KEY_GROQ, NVS_KEY_GEMINI, NVS_KEY_SERPER, NVS_KEY_OPENROUTER,
        NVS_KEY_TWELVEDATA, NVS_KEY_METEO, NVS_KEY_SPOTIFY_ID, NVS_KEY_SPOTIFY_SEC,
        NVS_KEY_TUYA_ID, NVS_KEY_TUYA_SEC, NVS_KEY_TUYA_REGION, NVS_KEY_TUYA_USER,
        NVS_KEY_ECOVACS_U, NVS_KEY_ECOVACS_P, NVS_KEY_ECOVACS_CC, NVS_KEY_ECOVACS_DEV,
        nullptr
    };
    for (int i = 0; keys[i]; ++i) {
        if (key == keys[i]) return true;
    }
    return false;
}

static String _device_status_json(const char* event = nullptr) {
    JsonDocument d;
    d["event"] = event ? event : "status";
    d["wifi"] = WiFi.status() == WL_CONNECTED ? "connected" : "disconnected";
    d["ssid"] = WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "";
    d["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
    d["heap"] = ESP.getFreeHeap();
    d["ble"] = _connected;
    String out;
    serializeJson(d, out);
    return out;
}

static void _publish_status(const char* event = nullptr) {
    _notify_json(_chr_dev_status, _device_status_json(event));
}

static void _scan_wifi_now() {
    JsonDocument d;
    JsonArray arr = d.to<JsonArray>();
    int n = WiFi.scanNetworks(false, true);
    int max_items = n > 16 ? 16 : n;
    for (int i = 0; i < max_items; ++i) {
        JsonObject o = arr.add<JsonObject>();
        o["ssid"] = WiFi.SSID(i);
        o["rssi"] = WiFi.RSSI(i);
        o["secured"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
        o["channel"] = WiFi.channel(i);
    }
    WiFi.scanDelete();
    _last_wifi_scan = "";
    serializeJson(d, _last_wifi_scan);
    _notify_json(_chr_wifi_scan, _last_wifi_scan);
}

static void _handle_agent_command(const String& val) {
    JsonDocument d;
    DeserializationError err = deserializeJson(d, val);
    if (err) {
        _notify_json(_chr_agent_sync, "{\"ok\":false,\"error\":\"json\"}");
        return;
    }

    String cmd = d["cmd"] | "";
    if (cmd == "set_api_key") {
        String key = d["key"] | "";
        String value = d["value"] | "";
        if (!_allowed_api_key(key)) {
            _notify_json(_chr_agent_sync, "{\"ok\":false,\"error\":\"key\"}");
            return;
        }
        bool ok = value.length() ? nvs_set_api_key(key.c_str(), value.c_str())
                                 : (nvs_clear_api_key(key.c_str()), true);
        String out = "{\"ok\":";
        out += ok ? "true" : "false";
        out += ",\"cmd\":\"set_api_key\",\"key\":\"";
        out += key;
        out += "\"}";
        _notify_json(_chr_agent_sync, out);
        _publish_status("nvs");
        return;
    }

    if (cmd == "list_api_keys") {
        char buf[512];
        nvs_list_api_keys_json(buf, sizeof(buf));
        String out = "{\"ok\":true,\"cmd\":\"list_api_keys\",\"keys\":";
        out += buf;
        out += "}";
        _notify_json(_chr_agent_sync, out);
        return;
    }

    if (cmd == "set_config") {
        String ns = d["ns"] | "";
        String key = d["key"] | "";
        String value = d["value"] | "";
        if (ns.isEmpty() || key.isEmpty()) {
            _notify_json(_chr_agent_sync, "{\"ok\":false,\"error\":\"config\"}");
            return;
        }
        bool ok = false;
        if (ns == "compagnon") {
            if (key == NVS_KEY_VOLUME || key == NVS_KEY_LAUNCHER_BG) {
                ok = cfg_set_u8(key.c_str(), (uint8_t)value.toInt());
            } else if (key == NVS_KEY_SILENT) {
                ok = nvs_set_bool(key.c_str(), value == "1" || value == "true");
            } else {
                ok = cfg_set_str(key.c_str(), value.c_str());
            }
        } else {
            ok = NvsStore::setString(ns.c_str(), key.c_str(), value);
        }
        _notify_json(_chr_agent_sync, ok ? "{\"ok\":true,\"cmd\":\"set_config\"}"
                                         : "{\"ok\":false,\"cmd\":\"set_config\"}");
        _publish_status("config");
        return;
    }

    if (cmd == "status") {
        _publish_status("status");
        _notify_json(_chr_agent_sync, "{\"ok\":true,\"cmd\":\"status\"}");
        return;
    }

    if (_on_agent) _on_agent(val);
}

static void _handle_gps_write(const String& val) {
    JsonDocument d;
    if (deserializeJson(d, val)) return;
    double lat = d["lat"] | 0.0;
    double lon = d["lon"] | 0.0;
    double alt = d["alt"] | 0.0;
    double speed = d["speed"] | 0.0;
    NvsStore::setString("gps", "last", val);
    Serial.printf("[BLE/GPS] lat=%.6f lon=%.6f alt=%.1f speed=%.1f\n",
                  lat, lon, alt, speed);
    _notify_json(_chr_gps, val);
}

class ServerCB : public NimBLEServerCallbacks {
    void onConnect   (NimBLEServer*, NimBLEConnInfo&) override {
        _connected = true;
        Serial.println("[BLE] Connecte");
        _publish_status("connect");
    }
    void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int) override {
        _connected = false;
        Serial.println("[BLE] Deconnecte");
        NimBLEDevice::startAdvertising();
    }
};

class CharCB : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* chr, NimBLEConnInfo&) override {
        String uuid = chr->getUUID().toString().c_str();
        String val  = String(chr->getValue().c_str());

        if (uuid == CHR_WIFI_PROV) {
            JsonDocument d;
            if (!deserializeJson(d, val)) {
                String ssid = d["ssid"] | "";
                String pass = d["pass"] | "";
                if (pass.isEmpty()) pass = d["password"] | "";
                WifiMgr::saveCredentials(ssid, pass);
                Serial.printf("[BLE] WiFi credentials saved: %s\n", ssid.c_str());
                _publish_status("wifi_saved");
            }
        }
        else if (uuid == CHR_WIFI_SCAN) _scan_wifi_now();
        else if (uuid == CHR_TEXT_INPUT && _on_text)  _on_text(val);
        else if (uuid == CHR_AGENT_SYNC) _handle_agent_command(val);
        else if (uuid == CHR_LLM_RELAY  && _on_llm)  _on_llm(val);
        else if (uuid == CHR_GPS) _handle_gps_write(val);
    }
};

static ServerCB _srv_cb;
static CharCB   _chr_cb;

void ble_init(TextInputCb on_text, AgentSyncCb on_agent, LlmRelayCb on_llm) {
    _on_text  = on_text;
    _on_agent = on_agent;
    _on_llm   = on_llm;

    Preferences p;
    p.begin("ble_config", true);
    String name = p.getString("device_name", "Compagnon");
    p.end();

    NimBLEDevice::init(name.c_str());
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    _server = NimBLEDevice::createServer();
    _server->setCallbacks(&_srv_cb);

    NimBLEService* svc = _server->createService(SVC_MAIN);

    auto make = [&](const char* uuid, uint32_t props) {
        return svc->createCharacteristic(uuid, props);
    };

    uint32_t RN  = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY;
    uint32_t W   = NIMBLE_PROPERTY::WRITE;
    uint32_t RWN = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
                 | NIMBLE_PROPERTY::NOTIFY;

    _chr_wifi_scan  = make(CHR_WIFI_SCAN,  RWN); _chr_wifi_scan->setCallbacks(&_chr_cb);
    _chr_wifi_prov  = make(CHR_WIFI_PROV,  W);  _chr_wifi_prov->setCallbacks(&_chr_cb);
    _chr_agent_sync = make(CHR_AGENT_SYNC, RWN); _chr_agent_sync->setCallbacks(&_chr_cb);
    _chr_text_input = make(CHR_TEXT_INPUT, W);   _chr_text_input->setCallbacks(&_chr_cb);
    _chr_llm_relay  = make(CHR_LLM_RELAY,  RWN); _chr_llm_relay->setCallbacks(&_chr_cb);
    _chr_dev_status = make(CHR_DEV_STATUS, RN);
    _chr_gps        = make(CHR_GPS,        RWN); _chr_gps->setCallbacks(&_chr_cb);
    _chr_wifi_scan->setValue(_last_wifi_scan.c_str());
    _chr_dev_status->setValue(_device_status_json("boot").c_str());

    // NimBLE 2.x : svc->start() est deprecie et sans effet.
    // Les services sont demarres implicitement par _server->start().
    _server->start();

    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->addServiceUUID(SVC_MAIN);
    adv->start();
    Serial.printf("[BLE] Advertising as '%s'\n", name.c_str());
}

void ble_notify_status(const String& json) {
    if (!_connected) return;
    _chr_dev_status->setValue(json.c_str()); _chr_dev_status->notify();
}
void ble_notify_gps(float lat, float lon, float alt) {
    if (!_connected) return;
    char buf[64];
    snprintf(buf, sizeof(buf),
             "{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.1f}", lat, lon, alt);
    _chr_gps->setValue(buf); _chr_gps->notify();
}
void ble_notify_agent_sync(const String& json) {
    if (!_connected) return;
    _chr_agent_sync->setValue(json.c_str()); _chr_agent_sync->notify();
}
void ble_notify_llm_relay(const String& json) {
    if (!_connected) return;
    _chr_llm_relay->setValue(json.c_str()); _chr_llm_relay->notify();
}
void ble_set_wifi_scan_result(const String& json) {
    _chr_wifi_scan->setValue(json.c_str());
    if (_connected) _chr_wifi_scan->notify();
}
bool ble_connected() { return _connected; }

} // namespace ble
