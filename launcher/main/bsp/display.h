// CO5300 AMOLED display bring-up (QSPI) for the ESP32-S3-Touch-AMOLED-2.16.
#pragma once

#include "esp_err.h"

// Bring up the QSPI bus + CO5300 panel and turn the display on.
// Requires the AXP2101 rails to be enabled first.
esp_err_t display_init(void);

// Render one frame of the breathing Orb for animation phase `phase` (radians)
// and push it to the panel. Call repeatedly with an advancing phase to animate.
esp_err_t display_render_orb(float phase);
