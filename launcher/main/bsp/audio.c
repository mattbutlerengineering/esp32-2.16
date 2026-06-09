// Audio I/O over I2S using esp_codec_dev: ES8311 codec (speaker) + ES7210 ADC
// (dual mic) share one I2S std port (MCLK/BCLK/WS common, DOUT->ES8311,
// DIN<-ES7210). Init values follow the xiaozhi-esp32 board reference for this
// board. The ESP32-S3 is the I2S master; both codecs are slaves.

#include "bsp/audio.h"
#include "bsp/board_config.h"

#include "driver/i2s_std.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "es8311_codec.h"
#include "es7210_adc.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "audio";

static esp_codec_dev_handle_t s_spk = NULL;  // ES8311 output
static esp_codec_dev_handle_t s_mic = NULL;  // ES7210 input

// Create the shared I2S std channels (tx for the speaker, rx for the mics). The
// codec data interface reconfigures slots/clock and enables them on open().
static esp_err_t i2s_bringup(i2s_chan_handle_t *tx, i2s_chan_handle_t *rx)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    // Send zeros (not the stale DMA buffer) when there is nothing to play —
    // otherwise the speaker loops the last recording forever after playback.
    chan_cfg.auto_clear = true;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, tx, rx), TAG, "new i2s channel");

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_RATE_HZ),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                        I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCLK_GPIO,
            .bclk = I2S_BCLK_GPIO,
            .ws = I2S_WS_GPIO,
            .dout = I2S_DOUT_GPIO,
            .din = I2S_DIN_GPIO,
            .invert_flags = {0},
        },
    };
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(*tx, &std_cfg), TAG, "init tx std");
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(*rx, &std_cfg), TAG, "init rx std");
    return ESP_OK;
}

esp_err_t audio_init(i2c_master_bus_handle_t bus)
{
    i2s_chan_handle_t tx = NULL, rx = NULL;
    ESP_RETURN_ON_ERROR(i2s_bringup(&tx, &rx), TAG, "i2s");

    audio_codec_i2s_cfg_t i2s_cfg = { .rx_handle = rx, .tx_handle = tx };
    const audio_codec_data_if_t *data_if = audio_codec_new_i2s_data(&i2s_cfg);
    ESP_RETURN_ON_FALSE(data_if, ESP_FAIL, TAG, "i2s data if");

    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();
    ESP_RETURN_ON_FALSE(gpio_if, ESP_FAIL, TAG, "gpio if");

    // Separate I2C control interface per codec (different addresses, same bus).
    audio_codec_i2c_cfg_t spk_i2c = {
        .port = BOARD_I2C_PORT, .addr = ES8311_CODEC_DEFAULT_ADDR, .bus_handle = bus,
    };
    const audio_codec_ctrl_if_t *spk_ctrl = audio_codec_new_i2c_ctrl(&spk_i2c);
    ESP_RETURN_ON_FALSE(spk_ctrl, ESP_FAIL, TAG, "es8311 i2c");

    audio_codec_i2c_cfg_t mic_i2c = {
        .port = BOARD_I2C_PORT, .addr = ES7210_CODEC_DEFAULT_ADDR, .bus_handle = bus,
    };
    const audio_codec_ctrl_if_t *mic_ctrl = audio_codec_new_i2c_ctrl(&mic_i2c);
    ESP_RETURN_ON_FALSE(mic_ctrl, ESP_FAIL, TAG, "es7210 i2c");

    es8311_codec_cfg_t es8311_cfg = {
        .ctrl_if = spk_ctrl,
        .gpio_if = gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .pa_pin = AUDIO_PA_GPIO,
        .pa_reverted = false,
        .master_mode = false,
        .use_mclk = true,
    };
    const audio_codec_if_t *es8311_if = es8311_codec_new(&es8311_cfg);
    ESP_RETURN_ON_FALSE(es8311_if, ESP_FAIL, TAG, "es8311 new");

    es7210_codec_cfg_t es7210_cfg = {
        .ctrl_if = mic_ctrl,
        .master_mode = false,
        .mic_selected = ES7120_SEL_MIC1,
        .mclk_src = ES7210_MCLK_FROM_PAD,
    };
    const audio_codec_if_t *es7210_if = es7210_codec_new(&es7210_cfg);
    ESP_RETURN_ON_FALSE(es7210_if, ESP_FAIL, TAG, "es7210 new");

    esp_codec_dev_cfg_t spk_dev = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT, .codec_if = es8311_if, .data_if = data_if,
    };
    s_spk = esp_codec_dev_new(&spk_dev);
    ESP_RETURN_ON_FALSE(s_spk, ESP_FAIL, TAG, "spk dev");

    esp_codec_dev_cfg_t mic_dev = {
        .dev_type = ESP_CODEC_DEV_TYPE_IN, .codec_if = es7210_if, .data_if = data_if,
    };
    s_mic = esp_codec_dev_new(&mic_dev);
    ESP_RETURN_ON_FALSE(s_mic, ESP_FAIL, TAG, "mic dev");

    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = AUDIO_BITS,
        .channel = 1,
        .sample_rate = AUDIO_RATE_HZ,
    };
    ESP_RETURN_ON_FALSE(esp_codec_dev_open(s_spk, &fs) == 0, ESP_FAIL, TAG, "spk open");
    ESP_RETURN_ON_FALSE(esp_codec_dev_open(s_mic, &fs) == 0, ESP_FAIL, TAG, "mic open");
    esp_codec_dev_set_out_vol(s_spk, 82);      // 0..100, clean level
    esp_codec_dev_set_in_gain(s_mic, 30.0f);   // dB

    ESP_LOGI(TAG, "ES8311 + ES7210 up (%d Hz mono, PA=GPIO%d)", AUDIO_RATE_HZ, AUDIO_PA_GPIO);
    return ESP_OK;
}

esp_err_t audio_record(int16_t *buf, size_t frames)
{
    ESP_RETURN_ON_FALSE(s_mic && buf, ESP_ERR_INVALID_STATE, TAG, "mic not ready");
    int ret = esp_codec_dev_read(s_mic, buf, (int)(frames * sizeof(int16_t)));
    return ret == 0 ? ESP_OK : ESP_FAIL;
}

esp_err_t audio_play(const int16_t *buf, size_t frames)
{
    ESP_RETURN_ON_FALSE(s_spk && buf, ESP_ERR_INVALID_STATE, TAG, "spk not ready");
    int ret = esp_codec_dev_write(s_spk, (void *)buf, (int)(frames * sizeof(int16_t)));
    return ret == 0 ? ESP_OK : ESP_FAIL;
}
