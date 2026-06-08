// I2C bus scanner for the Waveshare ESP32-S3-Touch-AMOLED-2.16.
//
// The board shares one I2C bus (SDA=GPIO15, SCL=GPIO14) across the touch
// controller, PMU, IMU, RTC, and audio codecs. This scans 0x08..0x77 and
// names the addresses we expect from the datasheet so we can confirm the
// real peripheral map against CLAUDE.md.

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#define I2C_PORT     I2C_NUM_0
#define PIN_SDA      15
#define PIN_SCL      14
#define PROBE_TIMEOUT_MS 50

static const char *TAG = "i2c_scan";

// Expected peripherals per the Waveshare spec — used only to annotate output.
static const char *known_device(uint8_t addr)
{
    switch (addr) {
        case 0x18: return "ES8311 audio codec";
        case 0x34: return "AXP2101 PMU/charger";
        case 0x40: return "ES7210 mic ADC";
        case 0x51: return "PCF85063 RTC";
        case 0x5A: return "CST9220 touch (typ.)";
        case 0x6B: return "QMI8658 IMU";
        default:   return NULL;
    }
}

void app_main(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT,
        .sda_io_num = PIN_SDA,
        .scl_io_num = PIN_SCL,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));

    while (1) {
        printf("\n--- I2C scan on SDA=GPIO%d SCL=GPIO%d ---\n", PIN_SDA, PIN_SCL);
        int found = 0;
        for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
            esp_err_t err = i2c_master_probe(bus, addr, PROBE_TIMEOUT_MS);
            if (err == ESP_OK) {
                const char *name = known_device(addr);
                printf("  0x%02X  %s\n", addr, name ? name : "(unknown)");
                found++;
            }
        }
        printf("--- %d device(s) found ---\n", found);
        if (found == 0) {
            ESP_LOGW(TAG, "No devices — check that the bus is on GPIO%d/%d", PIN_SDA, PIN_SCL);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
