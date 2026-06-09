// Launcher firmware entry point for the ESP32-S3-Touch-AMOLED-2.16.
//
// Brings up the foundation — shared I2C bus, AXP2101 rails, CO5300 AMOLED,
// CST9217 touch, ES8311/ES7210 audio, WiFi — then runs the interactive Orb.
// A tap starts one tap-to-talk cycle, orchestrated by the voice-session
// coordinator (#9): record the user's speech, POST it to the Gemini Bridge,
// and play back the spoken reply, driving the Orb FSM through listening ->
// thinking -> speaking -> idle (or -> error on failure, dismissed by a tap).

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#include "bsp/board_config.h"
#include "bsp/pmic_axp2101.h"
#include "bsp/display.h"
#include "bsp/touch_cst9217.h"
#include "bsp/audio.h"
#include "bsp/wifi.h"
#include "net/bridge_client.h"
#include "audio/wav.h"
#include "voice/voice_session.h"
#include "orb/orb_fsm.h"

#include <stdlib.h>

static const char *TAG = "launcher";

#define RECORD_SECONDS 3
#define RECORD_FRAMES  (AUDIO_RATE_HZ * RECORD_SECONDS)

static int16_t *s_rec = NULL;  // PSRAM capture buffer

// Per-session state shared with the voice-session seam callbacks.
typedef struct {
    size_t   captured;    // frames captured this cycle
    uint8_t *reply;       // downloaded reply WAV (freed after playback)
    size_t   reply_len;
    float    phase;       // animation phase frozen for this cycle's renders
} VoiceCtx;

// Parse a canonical 44-byte RIFF/PCM WAV (as the Bridge produces): sample rate
// at offset 24, PCM data at offset 44. Returns false if too short.
static bool parse_wav(const uint8_t *wav, size_t len,
                      const int16_t **pcm, size_t *frames, uint32_t *rate)
{
    if (len <= WAV_HEADER_SIZE) {
        return false;
    }
    uint32_t r = wav[24] | (wav[25] << 8) | (wav[26] << 16) | ((uint32_t)wav[27] << 24);
    if (r == 0) {
        return false;
    }
    *rate = r;
    *pcm = (const int16_t *)(wav + WAV_HEADER_SIZE);
    *frames = (len - WAV_HEADER_SIZE) / sizeof(int16_t);
    return true;
}

// --- voice-session seams (run while the FSM is in the matching state) ---

static Audio rec_seam(void *c)
{
    VoiceCtx *v = c;
    display_render_orb(ORB_LISTENING, v->phase);  // green
    ESP_LOGI(TAG, "listening: recording %d s", RECORD_SECONDS);
    audio_record(s_rec, RECORD_FRAMES);
    v->captured = RECORD_FRAMES;
    return (Audio){ .data = (const uint8_t *)s_rec, .len = v->captured * sizeof(int16_t) };
}

static BridgeReply send_seam(void *c, Audio captured)
{
    VoiceCtx *v = c;
    (void)captured;  // we POST straight from the capture buffer
    display_render_orb(ORB_THINKING, v->phase);  // amber
    if (!wifi_is_connected()) {
        ESP_LOGW(TAG, "thinking: WiFi not connected — cannot reach the Bridge");
        return (BridgeReply){ .ok = false };
    }
    ESP_LOGI(TAG, "thinking: POSTing %u frames to the Bridge", (unsigned)v->captured);
    if (bridge_talk(s_rec, v->captured, AUDIO_RATE_HZ, &v->reply, &v->reply_len) != ESP_OK) {
        return (BridgeReply){ .ok = false };
    }
    return (BridgeReply){ .ok = true, .audio = { .data = v->reply, .len = v->reply_len } };
}

static void play_seam(void *c, Audio reply)
{
    VoiceCtx *v = c;
    display_render_orb(ORB_SPEAKING, v->phase);  // blue
    const int16_t *pcm = NULL;
    size_t frames = 0;
    uint32_t rate = 0;
    if (parse_wav(reply.data, reply.len, &pcm, &frames, &rate)) {
        ESP_LOGI(TAG, "speaking: playing %u frames @ %u Hz", (unsigned)frames, (unsigned)rate);
        audio_play(pcm, frames, rate);
    } else {
        ESP_LOGE(TAG, "speaking: malformed reply WAV (%u bytes)", (unsigned)reply.len);
    }
    free(v->reply);
    v->reply = NULL;
}

static esp_err_t i2c_bus_init(i2c_master_bus_handle_t *out_bus)
{
    i2c_master_bus_config_t cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = BOARD_I2C_PORT,
        .sda_io_num = BOARD_I2C_SDA_GPIO,
        .scl_io_num = BOARD_I2C_SCL_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    return i2c_new_master_bus(&cfg, out_bus);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Orb Launcher booting on ESP32-S3-Touch-AMOLED-2.16");

    i2c_master_bus_handle_t i2c_bus;
    ESP_ERROR_CHECK(i2c_bus_init(&i2c_bus));
    ESP_LOGI(TAG, "shared I2C bus up (SDA=%d SCL=%d)", BOARD_I2C_SDA_GPIO, BOARD_I2C_SCL_GPIO);

    // Power foundation: nothing displays or plays until the AXP2101 rails are on.
    ESP_ERROR_CHECK(axp2101_power_on(i2c_bus));

    // Bring up display, touch, and audio (all on the shared I2C bus).
    ESP_ERROR_CHECK(display_init());
    ESP_ERROR_CHECK(touch_init(i2c_bus));
    ESP_ERROR_CHECK(audio_init(i2c_bus));

    // WiFi is best-effort. Without it the Bridge is unreachable and a tap ends
    // in the error state (red); the device still boots and animates.
    if (wifi_start() != ESP_OK) {
        ESP_LOGW(TAG, "WiFi unavailable — voice replies need the Bridge");
    }

    s_rec = heap_caps_malloc(RECORD_FRAMES * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    ESP_ERROR_CHECK(s_rec ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_LOGI(TAG, "interactive Orb ready — tap to talk");

    // TODO(#4): Orb home Mini-app + Mini-app registry/swipe nav.

    VoiceCtx vctx = {0};
    VoiceSession vs;
    voice_session_init(&vs, rec_seam, send_seam, play_seam, &vctx);

    float phase = 0.0f;
    bool was_touched = false;

    while (1) {
        uint16_t tx = 0, ty = 0;
        const bool touched = touch_poll(&tx, &ty);
        const bool tap = touched && !was_touched;
        was_touched = touched;

        const OrbState st = voice_session_state(&vs);
        if (tap && st == ORB_IDLE) {
            ESP_LOGI(TAG, "tap (%u,%u)", tx, ty);
            vctx.phase = phase;
            voice_session_tap(&vs);                       // -> listening
            if (voice_session_release(&vs) == ORB_ERROR) { // record -> send -> play
                ESP_LOGW(TAG, "voice cycle failed — tap to dismiss");
            }
            was_touched = false;  // ignore contact left over from the cycle
        } else if (tap && st == ORB_ERROR) {
            ESP_LOGI(TAG, "dismissing error");
            voice_session_init(&vs, rec_seam, send_seam, play_seam, &vctx);  // -> idle
            was_touched = false;
        }

        display_render_orb(voice_session_state(&vs), phase);  // idle breathing / error linger
        phase += 0.12f;  // ~2.6s per breath at this frame rate
        vTaskDelay(pdMS_TO_TICKS(33));
    }
}
