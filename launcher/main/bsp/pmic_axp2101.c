// AXP2101 rail bring-up. Register sequence ported from the xiaozhi-esp32 board
// file (Pmic ctor for esp32-s3-touch-amoled-2.16) — the proven values for this
// board. Set the DLDO/ALDO rails to 3.3V and enable them, plus charger config.

#include "bsp/pmic_axp2101.h"
#include "bsp/board_config.h"
#include "esp_log.h"

static const char *TAG = "axp2101";

// Each entry is a (register, value) write. Voltage encodings follow the AXP2101
// datasheet: e.g. reg 0x82 = (mV - 1500)/100, reg 0x92 = (mV - 500)/100.
typedef struct { uint8_t reg; uint8_t val; } axp_write_t;

static const axp_write_t k_init[] = {
    {0x22, 0b110},              // PWRON > OFFLEVEL as POWEROFF source enable
    {0x27, 0x10},               // hold 4s to power off
    {0x80, 0x01},               // enable DCDC group
    {0x90, 0x00},               // LDO enable cleared before configuring
    {0x91, 0x00},
    {0x82, (3300 - 1500) / 100},// rail -> 3.3V
    {0x92, (3300 - 500) / 100}, // rail -> 3.3V
    {0x90, 0x01},               // enable LDOs
    {0x64, 0x02},               // CV charger target 4.1V
    {0x61, 0x02},               // precharge current 50mA
    {0x62, 0x08},               // charge current 200mA
    {0x63, 0x01},               // termination current 25mA
};

esp_err_t axp2101_power_on(i2c_master_bus_handle_t bus)
{
    i2c_master_dev_handle_t dev;
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ADDR_AXP2101,
        .scl_speed_hz = 400000,
    };
    esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, &dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "add device: %s", esp_err_to_name(err));
        return err;
    }

    for (size_t i = 0; i < sizeof(k_init) / sizeof(k_init[0]); i++) {
        uint8_t buf[2] = {k_init[i].reg, k_init[i].val};
        err = i2c_master_transmit(dev, buf, sizeof(buf), 100);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "write reg 0x%02X: %s", k_init[i].reg, esp_err_to_name(err));
            i2c_master_bus_rm_device(dev);
            return err;
        }
    }

    ESP_LOGI(TAG, "rails enabled (display + audio powered)");
    i2c_master_bus_rm_device(dev);
    return ESP_OK;
}
