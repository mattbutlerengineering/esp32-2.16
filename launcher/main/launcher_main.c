// Launcher firmware entry point for the ESP32-S3-Touch-AMOLED-2.16.
//
// Brings up the foundation — shared I2C bus, AXP2101 rails, CO5300 AMOLED,
// CST9217 touch, QMI8658 IMU — then runs the Mini-app registry (#4) with the
// Ambient visualization as the foreground Mini-app: a tilt-warped plasma orb
// that breathes, follows the board's tilt, and ripples where you touch it.
//
// The voice-assistant path (audio, WiFi, Bridge client, voice session) still
// builds and is host-tested, but is no longer wired into the main loop — the
// project pivoted to the visualization demo. Re-register it as a Mini-app to
// bring it back.

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#include "bsp/board_config.h"
#include "bsp/pmic_axp2101.h"
#include "bsp/display.h"
#include "bsp/touch_cst9217.h"
#include "bsp/imu_qmi8658.h"
#include "app/miniapp.h"
#include "app/ambient_app.h"

static const char *TAG = "launcher";

#define FRAME_MS 33  // ~30 FPS
#define FRAME_DT ((float)FRAME_MS / 1000.0f)

static AmbientApp s_ambient;

// --- board reads -> Ambient app seams ---

static bool touch_seam(void *ctx, uint16_t *x, uint16_t *y) {
    (void)ctx;
    return touch_poll(x, y);
}

static void imu_seam(void *ctx, float *ax, float *ay) {
    (void)ctx;
    imu_read_accel(ax, ay);
}

// --- Mini-app lifecycle callbacks ---

static void ambient_enter(void *ctx) {
    (void)ctx;
    ESP_LOGI(TAG, "ambient orb active — touch for ripples, tilt to steer");
}

static void ambient_tick_cb(void *ctx) {
    ambient_app_tick(ctx, FRAME_DT);
}

static void ambient_render_cb(void *ctx) {
    uint16_t *fb = display_framebuffer();
    if (fb == NULL) {
        return;  // display not up; nothing to draw into
    }
    ambient_render(ambient_app_ambient(ctx), fb, LCD_WIDTH, LCD_HEIGHT);
    display_flush();
}

static const MiniApp k_ambient_miniapp = {
    .name = "ambient",
    .on_enter = ambient_enter,
    .on_tick = ambient_tick_cb,
    .render = ambient_render_cb,
    .ctx = &s_ambient,
};

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

    // Power foundation: nothing displays until the AXP2101 rails are on.
    ESP_ERROR_CHECK(axp2101_power_on(i2c_bus));

    ESP_ERROR_CHECK(display_init());
    ESP_ERROR_CHECK(touch_init(i2c_bus));

    // IMU is best-effort: without it the field stays level (no tilt warp).
    if (imu_init(i2c_bus) != ESP_OK) {
        ESP_LOGW(TAG, "IMU unavailable — ambient field won't react to tilt");
    }

    ambient_app_init(&s_ambient, touch_seam, NULL, imu_seam, NULL,
                     LCD_WIDTH, LCD_HEIGHT);

    static MiniAppRegistry s_registry;
    miniapp_registry_init(&s_registry);
    const int idx = miniapp_register(&s_registry, &k_ambient_miniapp);
    ESP_ERROR_CHECK(idx >= 0 ? ESP_OK : ESP_FAIL);
    miniapp_set_active(&s_registry, idx);

    while (1) {
        miniapp_tick(&s_registry);
        vTaskDelay(pdMS_TO_TICKS(FRAME_MS));
    }
}
