// Launcher firmware entry point for the ESP32-S3-Touch-AMOLED-2.16.
//
// Brings up the foundation everything else needs — shared I2C bus, AXP2101
// power rails, the CO5300 AMOLED, the CST9217 touch panel, and ES8311/ES7210
// audio — then runs the interactive Orb. A tap starts a voice cycle that drives
// the Orb state machine (idle -> listening -> thinking -> speaking -> idle) and
// renders each state as a distinct hue.
//
// The cycle is an audio loopback (#6): listening records from the mics, speaking
// plays it straight back through the speaker. Thinking is a brief placeholder —
// the Bridge round-trip (#9) replaces it later. Tap-to-talk is the real entry.

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
#include "orb/orb_fsm.h"

static const char *TAG = "launcher";

#define RECORD_SECONDS 3
#define RECORD_FRAMES  (AUDIO_RATE_HZ * RECORD_SECONDS)
#define THINK_MS       500  // placeholder pause until the Bridge lands (#9)

static int16_t *s_rec = NULL;  // PSRAM capture buffer, RECORD_FRAMES mono samples

static const char *state_name(OrbState s)
{
    switch (s) {
    case ORB_IDLE:      return "idle";
    case ORB_LISTENING: return "listening";
    case ORB_THINKING:  return "thinking";
    case ORB_SPEAKING:  return "speaking";
    case ORB_ERROR:     return "error";
    default:            return "?";
    }
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

// Apply an event, log the transition, and render the resulting Orb state so the
// new hue shows before any blocking work in that state. Returns the new state.
static OrbState orb_enter(OrbFsm *fsm, OrbEvent ev, float phase)
{
    OrbState before = orb_fsm_state(fsm);
    OrbState after = orb_fsm_handle(fsm, ev);
    if (after != before) {
        ESP_LOGI(TAG, "orb: %s -> %s", state_name(before), state_name(after));
    }
    display_render_orb(after, phase);
    return after;
}

// One full voice cycle: record from the mics (listening), brief think, then play
// it back through the speaker (speaking). Blocks for the duration of the cycle.
static void voice_cycle(OrbFsm *fsm, float phase)
{
    orb_enter(fsm, ORB_EV_TAP, phase);        // -> listening (green)
    ESP_LOGI(TAG, "recording %d s", RECORD_SECONDS);
    if (audio_record(s_rec, RECORD_FRAMES) != ESP_OK) {
        ESP_LOGE(TAG, "record failed");
    }

    orb_enter(fsm, ORB_EV_SEND, phase);       // -> thinking (amber)
    vTaskDelay(pdMS_TO_TICKS(THINK_MS));

    orb_enter(fsm, ORB_EV_REPLY_READY, phase); // -> speaking (blue)
    ESP_LOGI(TAG, "playing back");
    if (audio_play(s_rec, RECORD_FRAMES) != ESP_OK) {
        ESP_LOGE(TAG, "play failed");
    }

    orb_enter(fsm, ORB_EV_PLAYBACK_DONE, phase); // -> idle (cyan)
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

    s_rec = heap_caps_malloc(RECORD_FRAMES * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    ESP_ERROR_CHECK(s_rec ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_LOGI(TAG, "interactive Orb ready — tap to talk (records %d s, plays it back)", RECORD_SECONDS);

    // TODO(#9): Bridge client replaces the THINK pause with a real reply.
    // TODO(#4): Orb home Mini-app + Mini-app registry/swipe nav.

    OrbFsm fsm;
    orb_fsm_init(&fsm);

    float phase = 0.0f;
    bool was_touched = false;

    while (1) {
        // Edge-detect a tap (finger-down transition).
        uint16_t tx = 0, ty = 0;
        const bool touched = touch_poll(&tx, &ty);
        const bool tap = touched && !was_touched;
        was_touched = touched;

        // A tap from rest runs one record/playback voice cycle.
        if (tap && orb_fsm_state(&fsm) == ORB_IDLE) {
            ESP_LOGI(TAG, "tap (%u,%u)", tx, ty);
            voice_cycle(&fsm, phase);
            was_touched = false;  // ignore contact left over from the cycle
        }

        display_render_orb(ORB_IDLE, phase);  // idle breathing
        phase += 0.12f;  // ~2.6s per breath at this frame rate
        vTaskDelay(pdMS_TO_TICKS(33));
    }
}
