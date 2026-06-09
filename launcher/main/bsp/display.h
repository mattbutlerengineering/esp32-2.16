// CO5300 AMOLED display bring-up (QSPI) for the ESP32-S3-Touch-AMOLED-2.16.
#pragma once

#include "esp_err.h"
#include "orb/orb_fsm.h"

// Bring up the QSPI bus + CO5300 panel and turn the display on.
// Requires the AXP2101 rails to be enabled first.
esp_err_t display_init(void);

// Render one frame of the Orb for the given assistant `state` and animation
// `phase` (radians) and push it to the panel. The Orb breathes (brightness +
// size pulse) regardless of state; the hue tells the user which state it is in.
// Call repeatedly with an advancing phase to animate.
esp_err_t display_render_orb(OrbState state, float phase);
