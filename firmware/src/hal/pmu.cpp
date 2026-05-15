// ============================================================
// CompagnonV2 — HAL PMU — AXP2101 (XPowersLib)
// ============================================================
#include "pmu.h"

static XPowersPMU   _pmu;
static pmu_status_t _status = {false, false, 50, 3700, PMU_BAT_COLOR_GREEN};

// IRQ handler (IRAM_ATTR pour être exécuté depuis IRAM même en cache miss)
static volatile bool _pmu_irq = false;
IRAM_ATTR void pmu_irq_handler(void) {
    _pmu_irq = true;
}

bool hal_pmu_init(void) {
    bool ok = _pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL);
    if (!ok) {
        Serial.println("[HAL] PMU AXP2101 NOT found!");
        return false;
    }

    // Configuration des rails de puissance Waveshare AMOLED 2.16"
    // DCDC1 = 3.3V MCU
    _pmu.setDC1Voltage(3300);
    _pmu.enableDC1();
    // ALDO2 = 3.3V capteur / périphériques
    _pmu.setALDO2Voltage(3300);
    _pmu.enableALDO2();
    // ALDO3 = 3.3V SD card
    _pmu.setALDO3Voltage(3300);
    _pmu.enableALDO3();
    // ALDO4 = 3.3V AMOLED VDDIO
    _pmu.setALDO4Voltage(3300);
    _pmu.enableALDO4();
    // BLDO1 = 1.8V pour le RM67162 IOVDD
    _pmu.setBLDO1Voltage(1800);
    _pmu.enableBLDO1();

    // Charge rapide 500mA
    _pmu.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);
    _pmu.enableCharge();

    // IRQ batterie
    _pmu.clearIrqStatus();
    _pmu.enableBatChargeDoneIrq();
    _pmu.enableBatInsertIrq();
    _pmu.enableBatRemoveIrq();
    attachInterrupt(digitalPinToInterrupt(AXP_INT), pmu_irq_handler, FALLING);

    // Lecture initiale
    hal_pmu_tick();

    Serial.printf("[HAL] PMU AXP2101 init OK — bat %d%%\n", _status.battery_pct);
    return true;
}

void hal_pmu_tick(void) {
    if (_pmu_irq) {
        _pmu.getIrqStatus();
        _pmu.clearIrqStatus();
        _pmu_irq = false;
    }

    _status.charging     = _pmu.isCharging();
    _status.usb_present  = _pmu.isVbusIn();
    _status.battery_pct  = _pmu.getBatteryPercent();
    _status.voltage_mv   = _pmu.getBattVoltage();

    if (_status.battery_pct > PMU_BAT_OK) {
        _status.color = PMU_BAT_COLOR_GREEN;
    } else if (_status.battery_pct > PMU_BAT_LOW) {
        _status.color = PMU_BAT_COLOR_ORANGE;
    } else {
        _status.color = PMU_BAT_COLOR_RED;
    }
}

const pmu_status_t* hal_pmu_get_status(void) {
    return &_status;
}

void hal_pmu_power_off(void) {
    Serial.println("[HAL] Power OFF");
    delay(100);
    _pmu.shutdown();
}
