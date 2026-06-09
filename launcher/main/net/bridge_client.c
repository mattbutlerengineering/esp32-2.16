// HTTP client for the Gemini Bridge's POST /talk: send captured speech as a WAV,
// receive the spoken reply as a WAV. The Bridge (and Gemini behind it) does the
// STT + reasoning + TTS; the device only ships audio and plays what comes back.

#include "net/bridge_client.h"
#include "audio/wav.h"
#include "settings/config.h"

#include "esp_http_client.h"
#include "esp_heap_caps.h"
#include "esp_check.h"
#include "esp_log.h"

#include <string.h>

static const char *TAG = "bridge";

#define TALK_TIMEOUT_MS 20000
#define REPLY_CHUNK     (32 * 1024)

// Read the whole response body into a growable PSRAM buffer.
static esp_err_t read_reply(esp_http_client_handle_t client, int64_t hint,
                            uint8_t **out, size_t *out_len)
{
    size_t cap = (hint > 0) ? (size_t)hint : REPLY_CHUNK;
    uint8_t *buf = heap_caps_malloc(cap, MALLOC_CAP_SPIRAM);
    ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, TAG, "reply alloc");

    size_t got = 0;
    while (true) {
        if (got == cap) {
            cap *= 2;
            uint8_t *bigger = heap_caps_realloc(buf, cap, MALLOC_CAP_SPIRAM);
            if (!bigger) {
                free(buf);
                return ESP_ERR_NO_MEM;
            }
            buf = bigger;
        }
        int r = esp_http_client_read(client, (char *)buf + got, cap - got);
        if (r < 0) {
            free(buf);
            return ESP_FAIL;
        }
        if (r == 0) {
            break;  // complete
        }
        got += (size_t)r;
    }

    if (got == 0) {
        free(buf);
        return ESP_FAIL;
    }
    *out = buf;
    *out_len = got;
    return ESP_OK;
}

esp_err_t bridge_talk(const int16_t *pcm, size_t frames, uint32_t sample_rate,
                      uint8_t **reply_wav, size_t *reply_len)
{
    ESP_RETURN_ON_FALSE(pcm && reply_wav && reply_len, ESP_ERR_INVALID_ARG, TAG, "args");
    ESP_RETURN_ON_FALSE(config_valid_bridge_url(CONFIG_ORB_BRIDGE_URL), ESP_ERR_INVALID_ARG,
                        TAG, "bad ORB_BRIDGE_URL '%s'", CONFIG_ORB_BRIDGE_URL);

    // Frame the capture as a WAV (header + PCM) in one PSRAM buffer.
    const size_t data_bytes = frames * sizeof(int16_t);
    const size_t wav_len = WAV_HEADER_SIZE + data_bytes;
    uint8_t *wav = heap_caps_malloc(wav_len, MALLOC_CAP_SPIRAM);
    ESP_RETURN_ON_FALSE(wav, ESP_ERR_NO_MEM, TAG, "wav alloc");
    wav_write_header(wav, sample_rate, 1, 16, (uint32_t)data_bytes);
    memcpy(wav + WAV_HEADER_SIZE, pcm, data_bytes);

    char url[160];
    snprintf(url, sizeof(url), "%s/talk", CONFIG_ORB_BRIDGE_URL);

    esp_http_client_config_t cfg = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = TALK_TIMEOUT_MS,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        free(wav);
        return ESP_FAIL;
    }
    esp_http_client_set_header(client, "Content-Type", "audio/wav");

    esp_err_t err = esp_http_client_open(client, (int)wav_len);
    if (err == ESP_OK) {
        int w = esp_http_client_write(client, (const char *)wav, wav_len);
        err = (w == (int)wav_len) ? ESP_OK : ESP_FAIL;
    }
    free(wav);  // body has been sent (or open failed)
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "POST %s failed: %s", url, esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    int64_t clen = esp_http_client_fetch_headers(client);
    int status = esp_http_client_get_status_code(client);
    if (status != 200) {
        ESP_LOGE(TAG, "Bridge returned HTTP %d", status);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    err = read_reply(client, clen, reply_wav, reply_len);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "reply: %u bytes", (unsigned)*reply_len);
    }
    return err;
}
