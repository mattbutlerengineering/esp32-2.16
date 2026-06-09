// AXP2101 PMU bring-up for the ESP32-S3-Touch-AMOLED-2.16.
//
// The display and audio rails are gated by the AXP2101 — nothing lights up
// until these rails are enabled. This is the first thing the Launcher does.
#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"

// Configure and enable the AXP2101 rails that power the display and audio.
// `bus` is the shared I2C master bus; the AXP2101 lives at ADDR_AXP2101.
esp_err_t axp2101_power_on(i2c_master_bus_handle_t bus);
