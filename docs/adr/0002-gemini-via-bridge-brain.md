# 0002 — The Voice assistant's Brain is Gemini, reached via a Bridge

- Status: Accepted
- Date: 2026-06-07

## Context

The Voice assistant needs an AI brain for open-ended conversation. Hard
constraint from the user: it must be **free or near-free to run** — no paid
per-message API bills on an always-on gadget. Three options were weighed: an
on-device engine, a direct cloud call from the device, and a relay on the user's
own computer.

## Decision

The device reaches the **Brain** through a **Bridge** — a small process running
on the user's own computer. The default Brain is **Gemini** (free tier; natively
handles audio and TTS). The Brain is a pluggable interface
(`respond(audio) -> (text, audio)`); `ClaudeBrain` / `OllamaBrain` are later
swaps. v1 transport is half-duplex HTTP (`POST /talk`).

## Consequences

- $0 in normal use (Gemini free tier); no bill per interaction.
- Firmware stays "dumb" (audio + display); assistant behavior is iterated in
  Python on a laptop, not by reflashing the device.
- Brain is swappable without touching the device.
- Requires the user's computer to be on. A device-direct-to-Gemini mode is left
  open as a later option.

## Alternatives considered

- **On-device LLM** — rejected: the ESP32-S3 cannot run a real conversational
  model. (On-device ESP-SR can do offline wake word / fixed commands only — used
  later for activation, not as the brain.)
- **Device calls a paid API directly (Claude/OpenAI)** — rejected for v1: violates
  the free constraint. Claude is also text-only (no native audio) and has no free
  API tier, so it is a deliberate later swap, not the default.
- **Device calls Gemini directly** — deferred: leaner but pushes all audio/TLS/TTS
  glue onto the MCU; harder to build and iterate. Kept as a future mode.
