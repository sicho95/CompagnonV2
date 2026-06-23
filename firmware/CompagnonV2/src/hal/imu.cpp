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
static int  _candidate_orientation = -1;
static int  _candidate_count       = 0;
static bool _changed          = false;

namespace {
constexpr float IMU_MIN_AXIS_G = 0.45f;
constexpr float IMU_DOMINANCE_G = 0.16f;
constexpr int   IMU_STABLE_SAMPLES = 4;
}

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

    const float abs_x = fabsf(ax);
    const float abs_y = fabsf(ay);
    if (abs_x < IMU_MIN_AXIS_G && abs_y < IMU_MIN_AXIS_G) return;
    if (fabsf(abs_x - abs_y) < IMU_DOMINANCE_G) return;

    int ori;
    if (abs_x > abs_y) {
        ori = (ax > 0) ? 3 : 1;
    } else {
        ori = (ay < 0) ? 0 : 2;
    }

    if (ori != _candidate_orientation) {
        _candidate_orientation = ori;
        _candidate_count = 1;
        return;
    }

    if (_candidate_count < IMU_STABLE_SAMPLES) {
        _candidate_count++;
        return;
    }

    if (ori != _orientation) {
        _orientation = ori;
        _changed          = true;
    }
#endif
}

bool hal_imu_changed()     { return _changed; }
int  hal_imu_orientation() { return _orientation; }
