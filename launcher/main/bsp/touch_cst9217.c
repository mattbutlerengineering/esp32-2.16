// CST9217 capacitive touch bring-up over the shared I2C bus. Wraps the
// waveshare/esp_lcd_touch_cst9217 driver on top of espressif/esp_lcd_touch, so
// the rest of the firmware sees a tiny poll API instead of the panel-IO plumbing.

#include "bsp/touch_cst9217.h"
#include "bsp/board_config.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_cst9217.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "touch";

static esp_lcd_touch_handle_t s_tp = NULL;

esp_err_t touch_init(i2c_master_bus_handle_t bus)
{
    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_CST9217_CONFIG();
    // The new i2c_master panel-IO driver requires an explicit bus speed; the
    // CST9217 config macro predates it and leaves scl_speed_hz at 0.
    io_cfg.scl_speed_hz = 400000;  // CST9217 supports 400 kHz fast-mode
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(bus, &io_cfg, &io), TAG, "panel io");

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_WIDTH,
        .y_max = LCD_HEIGHT,
        .rst_gpio_num = TOUCH_RST_GPIO,
        .int_gpio_num = GPIO_NUM_NC,  // polling: INT not wired to an ISR
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    ESP_RETURN_ON_ERROR(esp_lcd_touch_new_i2c_cst9217(io, &tp_cfg, &s_tp), TAG, "new cst9217");

    ESP_LOGI(TAG, "CST9217 touch up (I2C 0x%02X, RST=%d)",
             ESP_LCD_TOUCH_IO_I2C_CST9217_ADDRESS, TOUCH_RST_GPIO);
    return ESP_OK;
}

bool touch_poll(uint16_t *x, uint16_t *y)
{
    if (s_tp == NULL) {
        return false;
    }
    if (esp_lcd_touch_read_data(s_tp) != ESP_OK) {
        return false;
    }

    esp_lcd_touch_point_data_t pt[1] = {0};
    uint8_t count = 0;
    if (esp_lcd_touch_get_data(s_tp, pt, &count, 1) != ESP_OK) {
        return false;
    }
    if (count > 0) {
        if (x != NULL) {
            *x = pt[0].x;
        }
        if (y != NULL) {
            *y = pt[0].y;
        }
        return true;
    }
    return false;
}
