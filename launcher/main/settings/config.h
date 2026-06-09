// Settings config: pure validation/clamping logic for the Settings Mini-app.
//
// The persisted values (brightness, volume, Bridge URL, WiFi creds) live in NVS
// (hardware), but the rules that keep them sane are pure logic and host-tested
// here so the device never stores or applies an out-of-range / malformed value.
#pragma once

#include <stdbool.h>

// Clamp a percentage (brightness/volume) into 0..100.
int config_clamp_percent(int value);

// True if `url` is a usable Bridge endpoint: http:// or https:// followed by a
// non-empty host.
bool config_valid_bridge_url(const char *url);
