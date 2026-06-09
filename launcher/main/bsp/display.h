// CO5300 AMOLED display bring-up (QSPI) for the ESP32-S3-Touch-AMOLED-2.16.
#pragma once

#include "esp_err.h"

// Bring up the QSPI bus + CO5300 panel and turn the display on.
// Requires the AXP2101 rails to be enabled first.
esp_err_t display_init(void);

// Draw the Orb placeholder: a filled circle on a dark background, pushed to the
// panel from a PSRAM framebuffer. Proves the display pipeline end-to-end.
esp_err_t display_draw_orb(void);
