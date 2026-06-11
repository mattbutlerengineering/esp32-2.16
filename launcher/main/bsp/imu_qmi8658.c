// QMI8658 accelerometer bring-up over the shared I2C bus. Register values per
// the QST QMI8658 datasheet; only the accel path is enabled (no gyro, no INT).

#include "bsp/imu_qmi8658.h"
#include "bsp/board_config.h"

#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "imu";

// Register map (QMI8658 datasheet)
#define REG_WHO_AM_I 0x00  // reads 0x05
#define REG_CTRL1    0x02  // bit6: register auto-increment for burst reads
#define REG_CTRL2    0x03  // accel: [6:4] full-scale, [3:0] ODR
#define REG_CTRL7    0x08  // enables: bit0 accel
#define REG_AX_L     0x35  // AX_L..AZ_H, 6 bytes, little-endian int16

#define WHO_AM_I_VAL    0x05
#define CTRL1_ADDR_AI   0x40
#define CTRL2_2G_250HZ  0x05  // ±2g full-scale, ~250 Hz ODR
#define CTRL7_ACCEL_EN  0x01
#define ACCEL_LSB_PER_G 16384.0f  // 16-bit, ±2g

static i2c_master_dev_handle_t s_dev = NULL;

static esp_err_t reg_write(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(s_dev, buf, sizeof(buf), 100);
}

static esp_err_t reg_read(uint8_t reg, uint8_t *out, size_t len) {
    return i2c_master_transmit_receive(s_dev, &reg, 1, out, len, 100);
}

esp_err_t imu_init(i2c_master_bus_handle_t bus) {
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ADDR_QMI8658,
        .scl_speed_hz = 400000,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &dev_cfg, &s_dev),
                        TAG, "add device");

    uint8_t who = 0;
    ESP_RETURN_ON_ERROR(reg_read(REG_WHO_AM_I, &who, 1), TAG, "who_am_i read");
    ESP_RETURN_ON_FALSE(who == WHO_AM_I_VAL, ESP_ERR_INVALID_RESPONSE, TAG,
                        "who_am_i 0x%02X != 0x%02X", who, WHO_AM_I_VAL);

    ESP_RETURN_ON_ERROR(reg_write(REG_CTRL1, CTRL1_ADDR_AI), TAG, "ctrl1");
    ESP_RETURN_ON_ERROR(reg_write(REG_CTRL2, CTRL2_2G_250HZ), TAG, "ctrl2");
    ESP_RETURN_ON_ERROR(reg_write(REG_CTRL7, CTRL7_ACCEL_EN), TAG, "ctrl7");

    ESP_LOGI(TAG, "QMI8658 up (I2C 0x%02X, accel ±2g)", ADDR_QMI8658);
    return ESP_OK;
}

void imu_read_accel(float *ax, float *ay) {
    *ax = 0.0f;
    *ay = 0.0f;
    if (s_dev == NULL) {
        return;
    }
    uint8_t raw[6];
    if (reg_read(REG_AX_L, raw, sizeof(raw)) != ESP_OK) {
        return;
    }
    *ax = (int16_t)(raw[0] | (raw[1] << 8)) / ACCEL_LSB_PER_G;
    *ay = (int16_t)(raw[2] | (raw[3] << 8)) / ACCEL_LSB_PER_G;
}
