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
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <ArduinoJson.h>
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

class ServerCB : public NimBLEServerCallbacks {
    void onConnect   (NimBLEServer*, NimBLEConnInfo&) override {
        _connected = true;
        Serial.println("[BLE] Connecte");
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
                Preferences p;
                p.begin("wifi_config", false);
                p.putString("ssid", d["ssid"] | "");
                p.putString("pass", d["pass"] | "");
                p.end();
                Serial.println("[BLE] WiFi credentials saved");
            }
        }
        else if (uuid == CHR_TEXT_INPUT && _on_text)  _on_text(val);
        else if (uuid == CHR_AGENT_SYNC && _on_agent) _on_agent(val);
        else if (uuid == CHR_LLM_RELAY  && _on_llm)  _on_llm(val);
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

    _chr_wifi_scan  = make(CHR_WIFI_SCAN,  RN);
    _chr_wifi_prov  = make(CHR_WIFI_PROV,  W);  _chr_wifi_prov->setCallbacks(&_chr_cb);
    _chr_agent_sync = make(CHR_AGENT_SYNC, RWN); _chr_agent_sync->setCallbacks(&_chr_cb);
    _chr_text_input = make(CHR_TEXT_INPUT, W);   _chr_text_input->setCallbacks(&_chr_cb);
    _chr_llm_relay  = make(CHR_LLM_RELAY,  RWN); _chr_llm_relay->setCallbacks(&_chr_cb);
    _chr_dev_status = make(CHR_DEV_STATUS, RN);
    _chr_gps        = make(CHR_GPS,        RN);

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
