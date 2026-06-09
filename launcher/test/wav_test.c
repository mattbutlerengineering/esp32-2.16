// Host unit tests for the WAV header builder. The device wraps recorded PCM as
// a WAV the Bridge can read; this asserts the 44-byte canonical PCM header,
// independent of any actual audio capture.

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "wav.h"

static int g_checks = 0;
#define CHECK(cond) do { assert(cond); g_checks++; } while (0)

static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static uint16_t le16(const uint8_t *p) {
    return (uint16_t)(p[0] | p[1] << 8);
}

// Magic markers identify it as a WAVE file with a data chunk of the given size.
static void test_riff_wave_data_markers(void) {
    uint8_t h[WAV_HEADER_SIZE];
    wav_write_header(h, 24000, 1, 16, 4800);

    CHECK(memcmp(h + 0, "RIFF", 4) == 0);
    CHECK(memcmp(h + 8, "WAVE", 4) == 0);
    CHECK(memcmp(h + 12, "fmt ", 4) == 0);
    CHECK(memcmp(h + 36, "data", 4) == 0);
    CHECK(le32(h + 40) == 4800);          // data chunk size = PCM byte count
    CHECK(le32(h + 4) == 36 + 4800);      // RIFF chunk size = 36 + data
}

// fmt chunk describes a 24 kHz mono 16-bit PCM stream with derived rates.
static void test_fmt_fields(void) {
    uint8_t h[WAV_HEADER_SIZE];
    wav_write_header(h, 24000, 1, 16, 0);

    CHECK(le32(h + 16) == 16);     // PCM fmt chunk size
    CHECK(le16(h + 20) == 1);      // format = PCM
    CHECK(le16(h + 22) == 1);      // channels
    CHECK(le32(h + 24) == 24000);  // sample rate
    CHECK(le16(h + 34) == 16);     // bits per sample
    CHECK(le16(h + 32) == 2);      // block align = channels * bytes/sample = 1*2
    CHECK(le32(h + 28) == 24000 * 2); // byte rate = sample_rate * block_align
}

// Stereo / different depth derive different block align + byte rate.
static void test_stereo_derived_rates(void) {
    uint8_t h[WAV_HEADER_SIZE];
    wav_write_header(h, 48000, 2, 16, 0);

    CHECK(le16(h + 32) == 4);          // 2 ch * 2 bytes
    CHECK(le32(h + 28) == 48000 * 4);  // byte rate
}

int main(void) {
    test_riff_wave_data_markers();
    test_fmt_fields();
    test_stereo_derived_rates();
    printf("wav: all %d checks passed\n", g_checks);
    return 0;
}
