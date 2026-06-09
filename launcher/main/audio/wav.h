// Canonical 44-byte WAV (RIFF/PCM) header builder.
//
// The device records raw PCM from the ES7210 mic ADC and POSTs it to the Bridge
// as a WAV; this writes the header the Bridge's decoder expects. Pure logic,
// host-unit-testable.
#pragma once

#include <stdint.h>

#define WAV_HEADER_SIZE 44

// Write a 44-byte little-endian RIFF/WAVE PCM header into `out` for an audio
// stream of `data_bytes` PCM bytes at the given sample rate / channel count /
// bit depth.
void wav_write_header(uint8_t out[WAV_HEADER_SIZE],
                      uint32_t sample_rate,
                      uint16_t channels,
                      uint16_t bits_per_sample,
                      uint32_t data_bytes);
