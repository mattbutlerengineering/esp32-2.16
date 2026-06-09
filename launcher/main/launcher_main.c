// Launcher firmware entry point for the ESP32-S3-Touch-AMOLED-2.16.
//
// Brings up the foundation everything else needs — shared I2C bus, AXP2101
// power rails, the CO5300 AMOLED, and the CST9217 touch panel — then runs the
// interactive Orb: a tap drives the Orb state machine (idle -> listening ->
// thinking -> speaking -> idle) and each state renders as a distinct Orb hue.
//
// The listening/thinking/speaking durations are driven here by a placeholder
// timeline. They stand in for the real events that land later: audio capture
// end (#6), the Bridge's reply (#9), and playback completion (#6). Tap-to-talk
// (ORB_EV_TAP) is already the real entry event.

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "bsp/board_config.h"
#include "bsp/pmic_axp2101.h"
#include "bsp/display.h"
#include "bsp/touch_cst9217.h"
#include "orb/orb_fsm.h"

static const char *TAG = "launcher";

// Placeholder durations for the voice round-trip, replaced by real audio/network
// events in #6/#9. Milliseconds.
#define LISTEN_MS 1500
#define THINK_MS  1200
#define SPEAK_MS  2000

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

static int64_t now_ms(void)
{
    return esp_timer_get_time() / 1000;
}

// Apply an event and log the resulting transition. Returns the new state.
static OrbState fsm_step(OrbFsm *fsm, OrbEvent ev)
{
    OrbState before = orb_fsm_state(fsm);
    OrbState after = orb_fsm_handle(fsm, ev);
    if (after != before) {
        ESP_LOGI(TAG, "orb: %s -> %s", state_name(before), state_name(after));
    }
    return after;
}

void app_main(void)
{
    ESP_LOGI(TAG, "Orb Launcher booting on ESP32-S3-Touch-AMOLED-2.16");

    i2c_master_bus_handle_t i2c_bus;
    ESP_ERROR_CHECK(i2c_bus_init(&i2c_bus));
    ESP_LOGI(TAG, "shared I2C bus up (SDA=%d SCL=%d)", BOARD_I2C_SDA_GPIO, BOARD_I2C_SCL_GPIO);

    // Power foundation: nothing displays or plays until the AXP2101 rails are on.
    ESP_ERROR_CHECK(axp2101_power_on(i2c_bus));

    // Bring up the CO5300 AMOLED and the CST9217 touch panel (shared I2C bus).
    ESP_ERROR_CHECK(display_init());
    ESP_ERROR_CHECK(touch_init(i2c_bus));
    ESP_LOGI(TAG, "interactive Orb ready — tap to talk");

    // TODO(#6): ES8311/ES7210 audio replaces the LISTEN/SPEAK timers.
    // TODO(#9): Bridge client replaces the THINK timer with a real reply.
    // TODO(#4): Orb home Mini-app + Mini-app registry/swipe nav.

    OrbFsm fsm;
    orb_fsm_init(&fsm);

    float phase = 0.0f;
    bool was_touched = false;
    int64_t t_state = now_ms();  // when the current active state began

    while (1) {
        const int64_t t = now_ms();

        // Edge-detect a tap (finger-down transition).
        uint16_t tx = 0, ty = 0;
        const bool touched = touch_poll(&tx, &ty);
        const bool tap = touched && !was_touched;
        was_touched = touched;

        if (tap) {
            ESP_LOGI(TAG, "tap (%u,%u)", tx, ty);
        }

        OrbState st = orb_fsm_state(&fsm);

        // A tap from rest begins listening.
        if (tap && st == ORB_IDLE) {
            st = fsm_step(&fsm, ORB_EV_TAP);
            t_state = t;
        }

        // Placeholder timeline driving the rest of the round-trip. Real audio
        // (#6) and network (#9) events will fire these transitions instead.
        switch (st) {
        case ORB_LISTENING:
            if (t - t_state >= LISTEN_MS) {
                st = fsm_step(&fsm, ORB_EV_SEND);
                t_state = t;
            }
            break;
        case ORB_THINKING:
            if (t - t_state >= THINK_MS) {
                st = fsm_step(&fsm, ORB_EV_REPLY_READY);
                t_state = t;
            }
            break;
        case ORB_SPEAKING:
            if (t - t_state >= SPEAK_MS) {
                st = fsm_step(&fsm, ORB_EV_PLAYBACK_DONE);
                t_state = t;
            }
            break;
        default:
            break;
        }

        display_render_orb(orb_fsm_state(&fsm), phase);
        phase += 0.12f;  // ~2.6s per breath at this frame rate
        vTaskDelay(pdMS_TO_TICKS(33));
    }
}
