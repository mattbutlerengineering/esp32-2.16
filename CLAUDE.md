# CLAUDE.md

## Hardware Target

This project targets the **Waveshare ESP32-S3-Touch-AMOLED-2.16** development board — a
2.16-inch round capacitive-touch AMOLED dev board built around the ESP32-S3.

### Core
- **SoC:** ESP32-S3R8 (Xtensa LX7 dual-core, up to 240 MHz), 512 KB SRAM, 384 KB ROM
- **PSRAM:** 8 MB (embedded, 3.3 V)
- **Flash:** 16 MB external NOR
- **Wireless:** 2.4 GHz Wi-Fi (802.11 b/g/n), Bluetooth 5 (LE); onboard chip antenna + IPEX
- **USB:** Type-C (native USB-Serial/JTAG)

#### Verified from silicon (esptool, 2026-06-07)
Read over `/dev/cu.usbmodem2101` from the connected board:
- Chip: **ESP32-S3 (QFN56) revision v0.2**, 40 MHz crystal
- Features: Wi-Fi, BT 5 (LE), Dual Core + LP Core, 240 MHz, **embedded PSRAM 8 MB (AP_3v3)**
- Flash: **16 MB**, quad (4 data lines) per eFuse, 3.3 V (mfr `0x20`, device `0x4018`)
- USB mode: USB-Serial/JTAG
- MAC: `a4:cb:8f:d7:56:c8`

> esptool isn't installed globally — read the chip with
> `uvx --from esptool esptool.py --port /dev/cu.usbmodem2101 chip-id` (and `flash-id`).

### Display & Touch
- **Panel:** 2.16" AMOLED, 480 × 480, 16.7M colors
- **Display driver:** CO5300 over **QSPI**
- **Touch:** CST9220 capacitive controller over **I2C** (addr `0x5A`, verified)

### Onboard Peripherals (all on the shared I2C bus)
All six addresses below were **confirmed on the live board** by an I2C scan
(`i2c_scanner/`, SDA=GPIO15 / SCL=GPIO14, 2026-06-07).

| Component | Part | I2C addr | Notes |
|-----------|------|----------|-------|
| Touch | CST9217 | `0x5A` | Capacitive touch (driver: CST9217, not CST9220) |
| Audio codec | ES8311 | `0x18` | Speaker / DAC path |
| PMU / charger | AXP2101 | `0x34` | Battery management + charging |
| Mic ADC | ES7210 | `0x40` | Dual-mic array, echo cancellation |
| RTC | PCF85063 | `0x51` | |
| IMU | QMI8658 | `0x6B` | 6-axis (accel + gyro), INT1 on GPIO13 |

- **Battery:** 3.7 V Li-ion via MX1.25 connector (charged through AXP2101)
- **Storage:** microSD (TF) card slot

### GPIO / pin map
Full pin map, extracted from the xiaozhi-esp32 board file for this exact board
(`main/boards/waveshare/esp32-s3-touch-amoled-2.16/config.h`) and mirrored in
`launcher/main/bsp/board_config.h`. **Confirm against the physical board** before
relying on it (issue #2 is HITL for this reason).

- **I2C bus:** SDA = **GPIO15**, SCL = **GPIO14** (shared: touch, PMU, IMU, RTC, codecs)
- **Display (CO5300, QSPI):** CS=12, PCLK=38, D0=4, D1=5, D2=6, D3=7, RST=39; no backlight pin (brightness via panel cmd `0x51`)
- **Touch (CST9217):** RST=40, INT=11
- **Audio (I2S, 24 kHz mono):** MCLK=42, WS=45, BCLK=9, DIN=10 (mic/ES7210), DOUT=8 (codec/ES8311), PA-enable=46
- **Buttons:** BOOT = GPIO0; Waveshare docs also mention a user button on GPIO18 (unverified)

> Caveat: the reference also initializes a TCA9554 I2C IO-expander (`0x20`), but
> `i2c_scanner` did NOT detect `0x20` on this board. Confirm on hardware whether
> this revision has the expander before routing reset/enable lines through it.

## Development

- **Frameworks supported:** ESP-IDF and Arduino IDE.
- Display work typically uses LVGL on top of the CO5300 QSPI driver.
- The shared I2C bus means touch, sensors, RTC, PMU, and audio all contend on GPIO14/15 —
  initialize the bus once and share the handle.

### Toolchain (this machine)
- ESP-IDF **v5.3.2** at `~/esp/esp-idf`; activate with `. ~/esp/esp-idf/export.sh`.
- `cmake`/`ninja` come from Homebrew (not bundled by IDF here) — required on PATH.
- Build/flash/monitor a project (target esp32s3):
  ```
  . ~/esp/esp-idf/export.sh
  idf.py set-target esp32s3        # one-time per project
  idf.py -p /dev/cu.usbmodem2101 flash monitor
  ```
- `i2c_scanner/` is a minimal ESP-IDF app that scans the I2C bus and prints the
  peripheral map above — use it to re-verify hardware after wiring changes.

## References
- Product page: https://www.waveshare.com/esp32-s3-touch-amoled-2.16.htm
- Wiki / docs: https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-2.16

## Agent skills

### Issue tracker

Issues and PRDs live in this repo's GitHub Issues (`mattbutlerengineering/esp32-2.16`), via the `gh` CLI. See `docs/agents/issue-tracker.md`.

### Triage labels

Five canonical triage roles; `ready-for-human` maps to this repo's `needs-human` label. See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: one root `CONTEXT.md` + `docs/adr/`. See `docs/agents/domain.md`.
