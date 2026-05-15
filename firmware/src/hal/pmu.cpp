// =============================================================
// CompagnonV2 — hal/pmu.cpp
// =============================================================
#include "pmu.h"

XPowersPMU pmu;
volatile bool pmu_irq_flag = false;

bool pmu_init() {
    if (!pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, PIN_IIC_SDA, PIN_IIC_SCL)) {
        Serial.println("[HAL] PMU AXP2101 NOT found!");
        return false;
    }

    pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    pmu.clearIrqStatus();

    // Tension de charge cible : 4.2V
    pmu.setChargeTargetVoltage(3);

    // ADC
    pmu.enableBattDetection();
    pmu.enableBattVoltageMeasure();
    pmu.enableVbusVoltageMeasure();
    pmu.enableSystemVoltageMeasure();
    pmu.enableTemperatureMeasure();

    // IRQ bouton PWR : court ET long
    pmu.enableIRQ(
        XPOWERS_AXP2101_PKEY_SHORT_IRQ |
        XPOWERS_AXP2101_PKEY_LONG_IRQ
    );

    Serial.println("[HAL] PMU AXP2101 init OK (SHORT+LONG IRQ actives)");
    return true;
}

pmu_event_t pmu_handle_irq() {
    // Polling flag (ou appel depuis ISR si pin INT câblée)
    if (!pmu_irq_flag) return PMU_EVT_NONE;
    pmu_irq_flag = false;

    uint32_t status = pmu.getIrqStatus();
    pmu.clearIrqStatus();

    if (pmu.isPekeyLongPressIrq()) {
        Serial.println("[PMU] PKEY LONG → poweroff");
        return PMU_EVT_PWR_LONG;
    }
    if (pmu.isPekeyShortPressIrq()) {
        Serial.println("[PMU] PKEY SHORT → toggle backlight");
        return PMU_EVT_PWR_SHORT;
    }
    return PMU_EVT_NONE;
}

void pmu_poweroff() {
    Serial.println("[PMU] Poweroff via AXP2101");
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
