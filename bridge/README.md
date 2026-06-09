# Orb Bridge

The computer-side **Bridge** for the Orb Voice assistant (see issue #3, PRD #1).

The device streams captured microphone audio to this service over your LAN; the
Bridge hands it to a pluggable **Brain**, and returns reply audio for the device
to play. The default Brain is **Gemini** (free tier); `ClaudeBrain` / `OllamaBrain`
can be added behind the same `Brain` interface without touching callers.

```
device  ──POST /talk (audio)──▶  Bridge  ──▶  Brain (Gemini)  ──▶  reply audio  ──▶  device
```

## Layout

- `orb_bridge/brain.py` — the `Brain` interface, `BrainReply`, and `GeminiBrain`
  (orchestration only; the network call is behind a client seam).
- `orb_bridge/gemini_client.py` — the real Gemini seam (audio-in reply + TTS).
- `orb_bridge/server.py` — Flask app exposing `POST /talk`.
- `orb_bridge/__main__.py` — wires it together and serves.
- `tests/` — pytest suite; runs with **no API key and no network** (Gemini mocked).

## Run it

```bash
cd bridge
uv venv && uv pip install -e ".[dev]"

# Free Gemini key: https://aistudio.google.com/apikey
export GEMINI_API_KEY=...

uv run python -m orb_bridge          # serves on 0.0.0.0:8080
```

Point the device's Bridge URL (set in the Settings Mini-app) at
`http://<your-computer-ip>:8080/talk`.

Config via env: `ORB_BRIDGE_HOST` (default `0.0.0.0`), `ORB_BRIDGE_PORT` (default `8080`).

### Try it without the device

```bash
curl -s -X POST --data-binary @sample.wav \
  -H 'Content-Type: application/octet-stream' \
  http://localhost:8080/talk --output reply.wav
```

## Test

```bash
uv run pytest        # no API key / network required
```

The suite asserts behavior, not the SDK wire format: given audio in, the Brain
returns reply audio; `POST /talk` returns the Brain's audio; a Brain/network
failure returns a legible `502` (never a silent success); empty audio returns `400`.
