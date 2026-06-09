// CST9217 capacitive touch bring-up (I2C) for the ESP32-S3-Touch-AMOLED-2.16.
#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"
#include <stdbool.h>
#include <stdint.h>

// Bring up the CST9217 on the shared I2C bus (addr 0x5A, RST=GPIO40). Polling
// mode — the INT line is not wired to an ISR. Requires the AXP2101 rails on.
esp_err_t touch_init(i2c_master_bus_handle_t bus);

// Poll the panel once. Returns true if a finger is currently down, writing its
// position (0..479) to *x and *y; returns false when nothing is touched.
bool touch_poll(uint16_t *x, uint16_t *y);
