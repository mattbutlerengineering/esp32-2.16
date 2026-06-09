# Orb Launcher

The flagship **Launcher** firmware for the Waveshare ESP32-S3-Touch-AMOLED-2.16
(see PRD #1). One firmware that boots into the **Orb** and hosts switchable
**Mini-apps** (Voice assistant, Motion, Settings).

## Scaffold status (issue #2 — HITL)

This is the foundation, not a finished Launcher. **Done** in this scaffold:

- Project builds for `esp32s3` with 8 MB octal PSRAM + 16 MB flash configured.
- Shared I2C bus bring-up (SDA=15 / SCL=14).
- **AXP2101 power rails enabled** — the prerequisite for the display and audio
  to power on (`main/bsp/pmic_axp2101.c`, register sequence ported from the
  xiaozhi-esp32 board file).
- Complete board pin map in `main/bsp/board_config.h`.

**TODO (needs hardware, tracked by later slices):**

- CO5300 display + LVGL bring-up over QSPI, draw the Orb (rest of #2).
- CST9217 touch + Mini-app registry + swipe nav (#4).
- ES8311/ES7210 audio I/O (#6).

## Why HITL

The pin map and AXP2101 sequence come from the xiaozhi-esp32 reference for this
exact board, not from a verified schematic. Notably, the reference uses a TCA9554
IO-expander (`0x20`) that `i2c_scanner` did **not** detect on this board — confirm
on hardware before completing the display bring-up. See `CLAUDE.md` → GPIO / pin map.

## Build / flash

```bash
. ~/esp/esp-idf/export.sh
idf.py set-target esp32s3          # one-time
idf.py -p /dev/cu.usbmodem2101 flash monitor
```

Expected at this stage: boots, brings up I2C, logs "rails enabled", then idles
(no display output yet).
