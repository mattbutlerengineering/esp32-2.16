# 0001 — The Launcher hosts Mini-apps as modules in one firmware

- Status: Accepted
- Date: 2026-06-07

## Context

The device runs a single firmware image at a time (an ESP32-S3 boots one app;
there is no OS that swaps independent firmware binaries). We want several
experiences on the device — a Voice assistant, a Motion toy, Settings, and more
later — and the user explicitly wanted "multiple apps on the device."

## Decision

Build one flagship firmware, the **Launcher**, that hosts multiple **Mini-apps**
as modules within it. A Mini-app registry dispatches a uniform lifecycle
(`on_enter`, `on_tick`, `on_exit`, `render`); the Launcher boots into the Orb
(the home Mini-app) and switches the foreground Mini-app on swipe/flick. Only one
Mini-app is foreground at a time; background tasks may run. Standalone Utilities
(e.g. `i2c_scanner/`) remain separate flash images, not Mini-apps.

## Consequences

- Adding a new experience = adding a Mini-app module, not a new firmware.
- All Mini-apps share the same display/touch/audio/network stack initialized once.
- "Multiple apps at once" is delivered via FreeRTOS tasks + a launcher shell, not
  by running multiple firmware images (which the hardware cannot do).
- The registry/lifecycle is pure logic and host-testable without hardware.

## Alternatives considered

- **Separate firmware per experience** — rejected: only one runs at a time, no
  in-device switching, and every app would re-implement board bring-up.
