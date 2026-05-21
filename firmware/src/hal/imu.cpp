// ============================================================
// CompagnonV2 — hal/imu.cpp
// QMI8658 accelerometre — orientation auto ecran
// ============================================================
#include "imu.h"
#include "../../include/pins.h"
#include <Arduino.h>
#include <Wire.h>

#if __has_include(<SensorQMI8658.hpp>)
  #include <SensorQMI8658.hpp>
  static SensorQMI8658* _qmi    = nullptr;
  static bool           _qmi_ok = false;
#endif

static int  _orientation      = 0;
static int  _prev_orientation = -1;
static bool _changed          = false;

void hal_imu_init() {
#if __has_include(<SensorQMI8658.hpp>)
    _qmi = new SensorQMI8658();
    _qmi_ok = _qmi->begin(Wire, QMI8658_I2C_ADDR, PIN_IMU_INT1, PIN_IMU_INT2);
    if (_qmi_ok) {
        _qmi->configAccelerometer(SensorQMI8658::ACC_RANGE_4G,
                                  SensorQMI8658::ACC_ODR_62_5Hz,
                                  SensorQMI8658::LPF_MODE_0);
        _qmi->enableAccelerometer();
        Serial.printf("[IMU] QMI8658 OK addr=0x%02X INT1=%d INT2=%d\n",
                      QMI8658_I2C_ADDR, PIN_IMU_INT1, PIN_IMU_INT2);
    } else {
        Serial.printf("[IMU] QMI8658 non detecte addr=0x%02X\n", QMI8658_I2C_ADDR);
        delete _qmi;
        _qmi = nullptr;
    }
#else
    Serial.println("[IMU] Stub — orientation fixe Portrait");
#endif
}

void hal_imu_tick() {
    _changed = false;
#if __has_include(<SensorQMI8658.hpp>)
    if (!_qmi_ok || !_qmi) return;
    float ax, ay, az;
    if (!_qmi->getAccelerometer(ax, ay, az)) return;
    int ori;
    if (fabsf(ax) > fabsf(ay)) {
        ori = (ax > 0) ? 3 : 1;
    } else {
        ori = (ay < 0) ? 0 : 2;
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
