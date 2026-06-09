// CO5300 AMOLED bring-up over QSPI. Init sequence + QSPI config ported from the
// xiaozhi-esp32 board file for this exact board. Draws the Orb placeholder
// directly to a PSRAM framebuffer (no LVGL yet) to prove the display works.

#include "bsp/display.h"
#include "bsp/board_config.h"

#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_co5300.h"
#include "esp_heap_caps.h"
#include "esp_check.h"
#include "esp_log.h"

#include <string.h>

static const char *TAG = "display";

#define LCD_HOST SPI2_HOST
#define LCD_BITS_PER_PIXEL 16

static esp_lcd_panel_handle_t s_panel = NULL;
static uint16_t *s_fb = NULL;  // 480x480 RGB565 framebuffer in PSRAM

// Vendor-specific init for this CO5300 panel (from the reference board file).
static const co5300_lcd_init_cmd_t k_vendor_init[] = {
    {0x11, (uint8_t[]){0x00}, 0, 600},  // sleep out
    {0xFE, (uint8_t[]){0x20}, 1, 0},
    {0x19, (uint8_t[]){0x10}, 1, 0},
    {0x1C, (uint8_t[]){0xA0}, 1, 0},
    {0xFE, (uint8_t[]){0x00}, 1, 0},
    {0xC4, (uint8_t[]){0x80}, 1, 0},
    {0x3A, (uint8_t[]){0x55}, 1, 0},  // 16bpp
    {0x35, (uint8_t[]){0x00}, 1, 0},
    {0x53, (uint8_t[]){0x20}, 1, 0},
    {0x51, (uint8_t[]){0xFF}, 1, 0},  // brightness max
    {0x63, (uint8_t[]){0xFF}, 1, 0},
    {0x2A, (uint8_t[]){0x00, 0x00, 0x01, 0xDF}, 4, 0},  // cols 0..479
    {0x2B, (uint8_t[]){0x00, 0x00, 0x01, 0xDF}, 4, 0},  // rows 0..479
    {0x36, (uint8_t[]){0xA0}, 1, 0},
    {0x29, (uint8_t[]){0x00}, 0, 600},  // display on
};

esp_err_t display_init(void) {
    spi_bus_config_t bus_cfg = {
        .sclk_io_num = LCD_QSPI_PCLK_GPIO,
        .data0_io_num = LCD_QSPI_DATA0_GPIO,
        .data1_io_num = LCD_QSPI_DATA1_GPIO,
        .data2_io_num = LCD_QSPI_DATA2_GPIO,
        .data3_io_num = LCD_QSPI_DATA3_GPIO,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t),
        .flags = SPICOMMON_BUSFLAG_QUAD,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(LCD_HOST, &bus_cfg, SPI_DMA_CH_AUTO),
                        TAG, "spi bus init");

    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_io_spi_config_t io_cfg =
        CO5300_PANEL_IO_QSPI_CONFIG(LCD_QSPI_CS_GPIO, NULL, NULL);
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi(LCD_HOST, &io_cfg, &io),
                        TAG, "panel io");

    const co5300_vendor_config_t vendor_cfg = {
        .init_cmds = k_vendor_init,
        .init_cmds_size = sizeof(k_vendor_init) / sizeof(k_vendor_init[0]),
        .flags = {.use_qspi_interface = 1},
    };
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = LCD_RST_GPIO,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = LCD_BITS_PER_PIXEL,
        .vendor_config = (void *)&vendor_cfg,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_co5300(io, &panel_cfg, &s_panel),
                        TAG, "new co5300");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel), TAG, "reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel), TAG, "init");
    esp_lcd_panel_invert_color(s_panel, false);
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_panel, true), TAG, "disp on");

    s_fb = heap_caps_malloc(LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t),
                            MALLOC_CAP_SPIRAM);
    ESP_RETURN_ON_FALSE(s_fb != NULL, ESP_ERR_NO_MEM, TAG, "framebuffer alloc");

    ESP_LOGI(TAG, "CO5300 up (%dx%d QSPI), framebuffer in PSRAM", LCD_WIDTH, LCD_HEIGHT);
    return ESP_OK;
}

// RGB565, byte-swapped for the panel's big-endian pixel order.
static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t c = (uint16_t)((r >> 3) << 11 | (g >> 2) << 5 | (b >> 3));
    return (uint16_t)(c << 8 | c >> 8);
}

esp_err_t display_draw_orb(void) {
    ESP_RETURN_ON_FALSE(s_panel && s_fb, ESP_ERR_INVALID_STATE, TAG, "not initialized");

    const uint16_t bg = rgb565(8, 10, 20);      // near-black, hint of blue
    const uint16_t orb = rgb565(0, 220, 230);   // bright cyan
    const int cx = LCD_WIDTH / 2, cy = LCD_HEIGHT / 2;
    const int r2 = 150 * 150;

    for (int y = 0; y < LCD_HEIGHT; y++) {
        for (int x = 0; x < LCD_WIDTH; x++) {
            int dx = x - cx, dy = y - cy;
            s_fb[y * LCD_WIDTH + x] = (dx * dx + dy * dy <= r2) ? orb : bg;
        }
    }

    // Push in horizontal bands — a single full-frame SPI transfer is too large.
    const int band = 48;  // rows per transfer: 480*48*2 = 46 KB
    for (int y = 0; y < LCD_HEIGHT; y += band) {
        int y_end = (y + band > LCD_HEIGHT) ? LCD_HEIGHT : y + band;
        esp_err_t err = esp_lcd_panel_draw_bitmap(
            s_panel, 0, y, LCD_WIDTH, y_end, &s_fb[y * LCD_WIDTH]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "draw band rows %d..%d: %s", y, y_end, esp_err_to_name(err));
            return err;
        }
    }
    return ESP_OK;
}
