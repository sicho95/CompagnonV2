#pragma once
// HAL IMU — QMI8658 (Waveshare ESP32-S3-Touch-AMOLED-2.16)
// Orientation : 0=Portrait, 1=Landscape-L, 2=Portrait-inv, 3=Landscape-R

#ifdef __cplusplus
extern "C" {
#endif

void hal_imu_init();
void hal_imu_tick();

// Retourne true si l'orientation a changé depuis le dernier appel à hal_imu_tick()
bool hal_imu_changed();

// Retourne l'indice d'orientation courant [0-3]
int  hal_imu_orientation();

#ifdef __cplusplus
}
#endif
