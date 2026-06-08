"""Run the Orb Bridge: `python -m orb_bridge` (or the `orb-bridge` script).

Wires the real GeminiClient into a GeminiBrain and serves it over HTTP so the
device can reach it on your LAN. Requires GEMINI_API_KEY (see gemini_client.py).
"""

from __future__ import annotations

import logging
import os

from orb_bridge.brain import GeminiBrain
from orb_bridge.gemini_client import GeminiClient
from orb_bridge.server import create_app


def main() -> None:
    logging.basicConfig(level=logging.INFO)
    host = os.environ.get("ORB_BRIDGE_HOST", "0.0.0.0")
    port = int(os.environ.get("ORB_BRIDGE_PORT", "8080"))

    brain = GeminiBrain(GeminiClient())
    app = create_app(brain)
    app.run(host=host, port=port)


if __name__ == "__main__":
    main()
