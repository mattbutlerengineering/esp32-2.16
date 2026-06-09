// Launcher firmware entry point for the ESP32-S3-Touch-AMOLED-2.16.
//
// Scaffold status (issue #2, HITL): this brings up the shared I2C bus and
// enables the AXP2101 power rails — the foundation everything else needs. The
// CO5300 display + LVGL, CST9217 touch, and ES8311/ES7210 audio bring-up are
// TODO and must be completed/verified on hardware using bsp/board_config.h and
// the xiaozhi-esp32 reference.

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#include "bsp/board_config.h"
#include "bsp/pmic_axp2101.h"
#include "bsp/display.h"

static const char *TAG = "launcher";

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

    // Bring up the CO5300 AMOLED and animate the breathing Orb.
    ESP_ERROR_CHECK(display_init());
    ESP_LOGI(TAG, "Orb breathing on the AMOLED");

    // TODO(#2): LVGL on top of the CO5300; CST9217 touch (#4); ES8311/ES7210 audio (#6).
    // TODO(#4): Orb home Mini-app + Mini-app registry/swipe nav.

    float phase = 0.0f;
    while (1) {
        display_render_orb(phase);
        phase += 0.12f;          // ~2.6s per breath at this frame rate
        vTaskDelay(pdMS_TO_TICKS(33));
    }
}
