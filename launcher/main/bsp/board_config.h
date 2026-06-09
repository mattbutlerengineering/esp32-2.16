// Board pin map for the Waveshare ESP32-S3-Touch-AMOLED-2.16.
//
// Pin values were extracted from the xiaozhi-esp32 board support file
// (main/boards/waveshare/esp32-s3-touch-amoled-2.16/config.h) — the proven
// reference for this exact board. CONFIRM against the physical board before
// relying on the display/audio pins (issue #2 is HITL for this reason).
//
// Discrepancy to verify on hardware: the reference initializes a TCA9554 I2C
// IO-expander (addr 0x20), but the i2c_scanner Utility did NOT detect 0x20 on
// this board (it found 0x18/0x34/0x40/0x51/0x5A/0x6B). Either this board
// revision omits the expander or it is unpowered at scan time. Confirm before
// routing any reset/enable lines through an expander.

#pragma once

// ---- Shared I2C bus (touch, PMU, IMU, RTC, audio codecs) ----
#define BOARD_I2C_PORT        0
#define BOARD_I2C_SDA_GPIO    15
#define BOARD_I2C_SCL_GPIO    14

// On-bus device addresses (verified by i2c_scanner)
#define ADDR_ES8311           0x18  // audio codec (speaker/DAC)
#define ADDR_AXP2101          0x34  // PMU / charger
#define ADDR_ES7210           0x40  // mic ADC
#define ADDR_PCF85063         0x51  // RTC
#define ADDR_CST9217_TOUCH    0x5A  // capacitive touch (driver: CST9217, not CST9220)
#define ADDR_QMI8658          0x6B  // 6-axis IMU

// ---- Display: CO5300 AMOLED over QSPI, 480x480 ----
#define LCD_WIDTH             480
#define LCD_HEIGHT            480
#define LCD_QSPI_CS_GPIO      12
#define LCD_QSPI_PCLK_GPIO    38
#define LCD_QSPI_DATA0_GPIO   4
#define LCD_QSPI_DATA1_GPIO   5
#define LCD_QSPI_DATA2_GPIO   6
#define LCD_QSPI_DATA3_GPIO   7
#define LCD_RST_GPIO          39
// No backlight pin: AMOLED brightness is set via panel command 0x51.

// ---- Touch: CST9217 over I2C ----
#define TOUCH_RST_GPIO        40
#define TOUCH_INT_GPIO        11

// ---- Audio: ES8311 (out) + ES7210 (in) over I2S, 24 kHz mono ----
#define AUDIO_SAMPLE_RATE     24000
#define I2S_MCLK_GPIO         42
#define I2S_WS_GPIO           45
#define I2S_BCLK_GPIO         9
#define I2S_DIN_GPIO          10  // from ES7210 mic ADC
#define I2S_DOUT_GPIO         8   // to ES8311 codec
#define AUDIO_PA_GPIO         46  // speaker amplifier enable

// ---- Buttons ----
#define BOOT_BUTTON_GPIO      0
// Waveshare docs also mention a user button on GPIO18 (unverified here).
