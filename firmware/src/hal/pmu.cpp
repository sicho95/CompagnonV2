// =============================================================
// CompagnonV2 — hal/pmu.cpp
// fix: XPowersAXP2101 etait une variable globale — son constructeur
//      s'executait avant setup()/Wire.begin() → crash Saved PC:0x4037f2e6
//      Remplace par pointeur statique initialise dans pmu_init().
// =============================================================
#include "pmu.h"
#include <Wire.h>
#include <Arduino.h>

// Pointeur — PAS de constructeur global, init dans pmu_init() uniquement
static XPowersAXP2101* _pmu = nullptr;

volatile bool pmu_irq_flag = false;
static pmu_long_press_cb_t _long_press_cb = nullptr;

namespace hal {

XPowersAXP2101* pmu_get() { return _pmu; }

void pmu_set_long_press_cb(pmu_long_press_cb_t cb) {
    _long_press_cb = cb;
}

bool pmu_init() {
    // Wire.begin() doit etre appele ici si pas encore fait
    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);

    _pmu = new XPowersAXP2101();
    if (!_pmu->begin(Wire, AXP2101_SLAVE_ADDRESS, PIN_IIC_SDA, PIN_IIC_SCL)) {
        Serial.println("[HAL] PMU AXP2101 NOT found!");
        delete _pmu;
        _pmu = nullptr;
        return false;
    }

    _pmu->disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    _pmu->clearIrqStatus();
    _pmu->setChargeTargetVoltage(3);
    _pmu->enableBattDetection();
    _pmu->enableBattVoltageMeasure();
    _pmu->enableVbusVoltageMeasure();
    _pmu->enableSystemVoltageMeasure();
    _pmu->enableTemperatureMeasure();
    _pmu->enableIRQ(
        XPOWERS_AXP2101_PKEY_SHORT_IRQ |
        XPOWERS_AXP2101_PKEY_LONG_IRQ
    );

    Serial.println("[HAL] PMU AXP2101 init OK");
    return true;
}

pmu_event_t pmu_handle_irq() {
    if (!pmu_irq_flag || !_pmu) return PMU_EVT_NONE;
    pmu_irq_flag = false;

    _pmu->getIrqStatus();
    _pmu->clearIrqStatus();

    if (_pmu->isPekeyLongPressIrq()) {
        Serial.println("[PMU] PKEY LONG -> poweroff");
        if (_long_press_cb) _long_press_cb();
        return PMU_EVT_PWR_LONG;
    }
    if (_pmu->isPekeyShortPressIrq()) {
        Serial.println("[PMU] PKEY SHORT -> backlight toggle");
        return PMU_EVT_PWR_SHORT;
    }
    return PMU_EVT_NONE;
}

void pmu_poweroff() {
    Serial.println("[PMU] Poweroff");
    if (_pmu) _pmu->shutdown();
}

int pmu_battery_percent() {
    if (!_pmu || !_pmu->isBatteryConnect()) return -1;
    return (int)_pmu->getBatteryPercent();
}

bool pmu_is_charging() {
    return _pmu ? _pmu->isCharging() : false;
}

uint16_t pmu_battery_voltage_mv() {
    return _pmu ? (uint16_t)_pmu->getBattVoltage() : 0;
}

} // namespace hal
