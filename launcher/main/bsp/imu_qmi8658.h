// QMI8658 6-axis IMU bring-up (I2C) for the ESP32-S3-Touch-AMOLED-2.16.
// Accelerometer only — enough to read the gravity vector for tilt input.
#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

// Bring up the QMI8658 on the shared I2C bus (addr 0x6B): verify WHO_AM_I and
// enable the accelerometer at ±2g. Polling mode — INT1 is not used.
esp_err_t imu_init(i2c_master_bus_handle_t bus);

// Read the gravity vector's screen-plane components (in g) into *ax,*ay.
// Writes 0,0 if the IMU is not initialized or the read fails, so callers
// keep animating with a level field. Axis signs/orientation are the chip's
// native frame — confirm against the physical board (HITL) and flip here.
void imu_read_accel(float *ax, float *ay);
