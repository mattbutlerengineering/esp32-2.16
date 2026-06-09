// Audio I/O bring-up for the ESP32-S3-Touch-AMOLED-2.16: ES8311 codec (speaker)
// and ES7210 dual-mic ADC over a shared I2S port, fronted by a tiny record/play
// API. 16 kHz mono, 16-bit — consistent with the Bridge's expected input.
#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"
#include <stdint.h>
#include <stddef.h>

// Sample format. Mono, 16-bit signed PCM at 16 kHz (matches the Gemini Bridge).
#define AUDIO_RATE_HZ      16000
#define AUDIO_BITS         16

// Bring up the I2S port + ES8311 (out) and ES7210 (in) on the shared I2C bus.
// Requires the AXP2101 rails enabled first. Enables the speaker PA (GPIO46).
esp_err_t audio_init(i2c_master_bus_handle_t bus);

// Capture exactly `frames` mono 16-bit samples from the mics into `buf`
// (blocking until the buffer is full). `buf` must hold frames*2 bytes.
esp_err_t audio_record(int16_t *buf, size_t frames);

// Play `frames` mono 16-bit samples from `buf` through the speaker (blocking
// until the buffer has been pushed to the codec).
esp_err_t audio_play(const int16_t *buf, size_t frames);
