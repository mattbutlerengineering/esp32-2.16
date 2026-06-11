// CO5300 AMOLED display bring-up (QSPI) for the ESP32-S3-Touch-AMOLED-2.16.
#pragma once

#include "esp_err.h"
#include "orb/orb_fsm.h"

#include <stdint.h>

// Bring up the QSPI bus + CO5300 panel and turn the display on.
// Requires the AXP2101 rails to be enabled first.
esp_err_t display_init(void);

// Render one frame of the Orb for the given assistant `state` and animation
// `phase` (radians) and push it to the panel. The Orb breathes (brightness +
// size pulse) regardless of state; the hue tells the user which state it is in.
// Call repeatedly with an advancing phase to animate.
esp_err_t display_render_orb(OrbState state, float phase);

// The 480x480 RGB565 framebuffer (PSRAM), for Mini-apps that draw their own
// pixels. NULL until display_init() succeeds. Pixels are in panel byte order.
uint16_t *display_framebuffer(void);

// Push the framebuffer to the panel (banded QSPI transfers).
esp_err_t display_flush(void);
