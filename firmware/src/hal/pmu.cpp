// =============================================================
// CompagnonV2 — hal/pmu.cpp
// =============================================================
#include "pmu.h"

XPowersPMU pmu;
volatile bool pmu_irq_flag = false;

static void IRAM_ATTR _pmu_isr() {
    pmu_irq_flag = true;
}

bool pmu_init() {
    if (!pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, PIN_IIC_SDA, PIN_IIC_SCL)) {
        Serial.println("[HAL] PMU AXP2101 NOT found!");
        return false;
    }

    // Désactive toutes les IRQ, repart proprement
    pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    pmu.clearIrqStatus();

    // Tension de charge cible : 4.2V (index 3 dans XPowersLib)
    pmu.setChargeTargetVoltage(3);

    // ADC
    pmu.enableBattDetection();
    pmu.enableBattVoltageMeasure();
    pmu.enableVbusVoltageMeasure();
    pmu.enableSystemVoltageMeasure();
    pmu.enableTemperatureMeasure();

    // IRQ bouton POWER (court)
    pmu.enableIRQ(XPOWERS_AXP2101_PKEY_SHORT_IRQ);

    // Attache ISR sur pin PMU INT (sur cette carte, l'INT AXP est connecté
    // à la même ligne I2C — pas de pin INT dédiée visible dans pin_config.h
    // Waveshare. On utilise donc le polling dans pmu_handle_irq() si pas
    // de pin INT disponible. Décommenter si vous câblez une pin INT externe.)
    // attachInterrupt(PIN_PMU_INT, _pmu_isr, FALLING);

    Serial.println("[HAL] PMU AXP2101 init OK");
    return true;
}

void pmu_handle_irq() {
    // Polling fallback (ou appel depuis ISR si pin INT câblée)
    if (pmu_irq_flag) {
        pmu_irq_flag = false;
        pmu.clearIrqStatus();
    }
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
