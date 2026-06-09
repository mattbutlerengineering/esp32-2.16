#include "wav.h"

static void put_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static void put_u16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >> 8);
}

void wav_write_header(uint8_t out[WAV_HEADER_SIZE],
                      uint32_t sample_rate,
                      uint16_t channels,
                      uint16_t bits_per_sample,
                      uint32_t data_bytes) {
    const uint16_t block_align = (uint16_t)(channels * (bits_per_sample / 8));
    const uint32_t byte_rate = sample_rate * block_align;

    // RIFF chunk
    out[0] = 'R'; out[1] = 'I'; out[2] = 'F'; out[3] = 'F';
    put_u32(out + 4, 36 + data_bytes);
    out[8] = 'W'; out[9] = 'A'; out[10] = 'V'; out[11] = 'E';

    // fmt subchunk (PCM)
    out[12] = 'f'; out[13] = 'm'; out[14] = 't'; out[15] = ' ';
    put_u32(out + 16, 16);          // PCM fmt chunk size
    put_u16(out + 20, 1);           // audio format = PCM
    put_u16(out + 22, channels);
    put_u32(out + 24, sample_rate);
    put_u32(out + 28, byte_rate);
    put_u16(out + 32, block_align);
    put_u16(out + 34, bits_per_sample);

    // data subchunk
    out[36] = 'd'; out[37] = 'a'; out[38] = 't'; out[39] = 'a';
    put_u32(out + 40, data_bytes);
}
