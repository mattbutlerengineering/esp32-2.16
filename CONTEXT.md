# ESP32-2.16 Launcher

Firmware for the Waveshare ESP32-S3-Touch-AMOLED-2.16 round display board. The
flagship is a **Launcher** — a single firmware that hosts several switchable
**Mini-apps** on the round AMOLED. The repo also holds standalone bring-up
utilities (e.g. the I2C scanner) that are not part of the Launcher.

## Language

**Launcher**:
The single flagship firmware image that boots, draws the home screen, and hosts
Mini-apps. There is exactly one running on the device at a time.
_Avoid_: OS, shell, home (as a noun for the whole firmware), launcher-app

**Mini-app**:
A self-contained mode hosted inside the Launcher (clock, assistant, weather,
etc.). Mini-apps are modules within one firmware, not separate flash images —
only the Launcher's foreground Mini-app is visible, but background tasks may run.
_Avoid_: app (ambiguous with the firmware), face, tile, screen, widget

**Utility**:
A standalone ESP-IDF project in the repo that is flashed on its own for
hardware bring-up or diagnostics (e.g. `i2c_scanner/`). Not hosted by the Launcher.
_Avoid_: tool, demo, test app

**Orb**:
The Launcher's home/resting screen: a living, animated presence on the black
AMOLED that idles, listens, and reacts. It is also the visual body of the Voice
assistant — engaging the assistant does not leave the Orb.
_Avoid_: avatar, blob, face, home screen, wallpaper

**Voice assistant**:
The flagship Mini-app, embodied by the Orb. Captures speech from the mic array,
gets a spoken response from an AI brain, and plays it back. The board's showpiece.
_Avoid_: chatbot, agent, Siri/Alexa (as generic nouns), voice app
