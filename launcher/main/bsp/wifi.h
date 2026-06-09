// WiFi station bring-up for the Orb. Credentials come from NVS, seeded from
// Kconfig (ORB_WIFI_SSID/PASSWORD) on first boot so secrets never live in
// source. Connection is asynchronous — the Orb keeps animating while it joins.
#pragma once

#include "esp_err.h"
#include <stdbool.h>

// Initialize NVS + the WiFi station and start connecting (non-blocking). Returns
// ESP_OK once the station is started, ESP_ERR_INVALID_STATE if no SSID is
// configured (device stays offline). Connection progress is logged; poll
// wifi_is_connected() for status.
esp_err_t wifi_start(void);

// True once the station has acquired an IP address.
bool wifi_is_connected(void);
