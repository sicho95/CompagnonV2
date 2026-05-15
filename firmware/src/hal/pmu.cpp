// ============================================================
// CompagnonV2 — hal/pmu.cpp
// AXP2101 via XPowersLib 0.2.x
// ============================================================
#include "pmu.h"

static XPowersPMU s_pmu;
static BatteryInfo s_batt_info = {0, 0.0f, false, false};
static portMUX_TYPE s_batt_mux = portMUX_INITIALIZER_UNLOCKED;

// ── IRQ PMU ────────────────────────────────────────────────────────────────
static volatile bool s_pmu_irq = false;

static void IRAM_ATTR _pmu_irq_handler() {
    s_pmu_irq = true;
}

// ── Init ───────────────────────────────────────────────────────────────────
bool pmu_init() {
    if (!s_pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
        Serial.println("[PMU] ERREUR: AXP2101 non détecté");
        return false;
    }

    // Tension DCDC et LDO pour l'écran AMOLED et les périphériques
    s_pmu.setPowerChannelVoltage(XPOWERS_DCDC1, 3300);  // VDD3.3
    s_pmu.enablePowerOutput(XPOWERS_DCDC1);
    s_pmu.setPowerChannelVoltage(XPOWERS_ALDO2, 1800);  // VDDIO 1.8V
    s_pmu.enablePowerOutput(XPOWERS_ALDO2);
    s_pmu.setPowerChannelVoltage(XPOWERS_ALDO3, 3300);  // LCD power
    s_pmu.enablePowerOutput(XPOWERS_ALDO3);

    // Courant de charge 500 mA
    s_pmu.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);

    // IRQ sur broche AXP_INT
    s_pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    s_pmu.enableIRQ(XPOWERS_AXP2101_BAT_INSERT_IRQ |
                    XPOWERS_AXP2101_BAT_REMOVE_IRQ |
                    XPOWERS_AXP2101_VBUS_INSERT_IRQ |
                    XPOWERS_AXP2101_VBUS_REMOVE_IRQ);
    s_pmu.clearIRQ();

    pinMode(AXP_INT, INPUT);
    attachInterrupt(digitalPinToInterrupt(AXP_INT), _pmu_irq_handler, FALLING);

    Serial.println("[PMU] AXP2101 init OK");
    return true;
}

// ── Tick ~1s ───────────────────────────────────────────────────────────────
void pmu_tick() {
    if (s_pmu_irq) {
        s_pmu.clearIRQ();
        s_pmu_irq = false;
    }

    BatteryInfo b;
    b.percent     = (uint8_t)s_pmu.getBatteryPercent();
    b.voltage     = s_pmu.getBattVoltage();
    b.charging    = s_pmu.isCharging();
    b.usb_present = s_pmu.isVbusIn();

    portENTER_CRITICAL(&s_batt_mux);
    s_batt_info = b;
    portEXIT_CRITICAL(&s_batt_mux);
}

// ── Getter thread-safe ─────────────────────────────────────────────────────
BatteryInfo pmu_get_battery() {
    BatteryInfo b;
    portENTER_CRITICAL(&s_batt_mux);
    b = s_batt_info;
    portEXIT_CRITICAL(&s_batt_mux);
    return b;
}

void pmu_set_charging_led(bool on) {
    // LED de charge via CHG_LED AXP2101
    on ? s_pmu.enableChargingLed() : s_pmu.disableChargingLed();
}
