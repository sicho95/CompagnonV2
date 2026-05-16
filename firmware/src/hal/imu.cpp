#include "imu.h"
#include <Arduino.h>
// Driver QMI8658 — I2C adresse 0x6B (SD0 = VCC sur Waveshare 2.16")
// Utilise la librairie SensorLib (lewisxhe/SensorsLib) si disponible,
// sinon stub statique Portrait.

#if __has_include(<SensorQMI8658.hpp>)
  #include <SensorQMI8658.hpp>
  static SensorQMI8658 _qmi;
  static bool _qmi_ok = false;
#else
  #warning "SensorsLib non installée — IMU en mode stub (orientation fixe Portrait)"
#endif

static int  _orientation     = 0;   // Portrait par défaut
static int  _prev_orientation = -1;
static bool _changed         = false;

void hal_imu_init() {
#if __has_include(<SensorQMI8658.hpp>)
    // SDA=GPIO6, SCL=GPIO7 sur Waveshare AMOLED 2.16"
    _qmi_ok = _qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, 6, 7);
    if (_qmi_ok) {
        _qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G,
                                  SensorQMI8658::ACC_ODR_62_5Hz,
                                  SensorQMI8658::LPF_MODE_0);
        _qmi.enableAccelerometer();
        Serial.println("[IMU] QMI8658 OK");
    } else {
        Serial.println("[IMU] QMI8658 non détecté, orientation fixe");
    }
#else
    Serial.println("[IMU] Stub — orientation fixe Portrait");
#endif
}

void hal_imu_tick() {
    _changed = false;
#if __has_include(<SensorQMI8658.hpp>)
    if (!_qmi_ok) return;
    float ax, ay, az;
    if (!_qmi.getAccelerometer(ax, ay, az)) return;
    int ori;
    if (fabsf(ax) > fabsf(ay)) {
        ori = (ax > 0) ? 3 : 1;  // Landscape R ou L
    } else {
        ori = (ay < 0) ? 0 : 2;  // Portrait ou Portrait inversé
    }
    if (ori != _prev_orientation) {
        _prev_orientation = ori;
        _orientation      = ori;
        _changed          = true;
    }
#endif
}

bool hal_imu_changed()     { return _changed; }
int  hal_imu_orientation() { return _orientation; }
