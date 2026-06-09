// Device-side client for the Gemini Bridge. Frames captured mic PCM as a WAV,
// POSTs it to <ORB_BRIDGE_URL>/talk, and returns the reply WAV the Bridge sends
// back (Gemini's spoken answer). Requires WiFi to be connected.
#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

// POST `frames` of 16-bit mono PCM (at `sample_rate`) to the Bridge's /talk
// endpoint as a WAV. On success, allocates the reply WAV bytes into *reply_wav
// (caller must free()) and sets *reply_len. Returns ESP_OK only on HTTP 200 with
// a non-empty body.
esp_err_t bridge_talk(const int16_t *pcm, size_t frames, uint32_t sample_rate,
                      uint8_t **reply_wav, size_t *reply_len);
