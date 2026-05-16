// =============================================================
// CompagnonV2 — hal/pmu.cpp
// =============================================================
#include "pmu.h"

XPowersAXP2101 pmu;
volatile bool pmu_irq_flag = false;

static pmu_long_press_cb_t _long_press_cb = nullptr;

void pmu_set_long_press_cb(pmu_long_press_cb_t cb) {
    _long_press_cb = cb;
}

bool pmu_init() {
    if (!pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, PIN_IIC_SDA, PIN_IIC_SCL)) {
        Serial.println("[HAL] PMU AXP2101 NOT found!");
        return false;
    }

    pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    pmu.clearIrqStatus();
    pmu.setChargeTargetVoltage(3);
    pmu.enableBattDetection();
    pmu.enableBattVoltageMeasure();
    pmu.enableVbusVoltageMeasure();
    pmu.enableSystemVoltageMeasure();
    pmu.enableTemperatureMeasure();
    pmu.enableIRQ(
        XPOWERS_AXP2101_PKEY_SHORT_IRQ |
        XPOWERS_AXP2101_PKEY_LONG_IRQ
    );

    Serial.println("[HAL] PMU AXP2101 init OK");
    return true;
}

pmu_event_t pmu_handle_irq() {
    if (!pmu_irq_flag) return PMU_EVT_NONE;
    pmu_irq_flag = false;

    pmu.getIrqStatus();
    pmu.clearIrqStatus();

    if (pmu.isPekeyLongPressIrq()) {
        Serial.println("[PMU] PKEY LONG → poweroff");
        if (_long_press_cb) _long_press_cb();
        return PMU_EVT_PWR_LONG;
    }
    if (pmu.isPekeyShortPressIrq()) {
        Serial.println("[PMU] PKEY SHORT → backlight toggle");
        return PMU_EVT_PWR_SHORT;
    }
    return PMU_EVT_NONE;
}

void pmu_poweroff() {
    Serial.println("[PMU] Poweroff");
    pmu.shutdown();
}

int pmu_battery_percent() {
    if (!pmu.isBatteryConnect()) return -1;
    return (int)pmu.getBatteryPercent();
}

bool pmu_is_charging() {
    return pmu.isCharging();
}

uint16_t pmu_battery_voltage_mv() {
    return (uint16_t)pmu.getBattVoltage();
}
