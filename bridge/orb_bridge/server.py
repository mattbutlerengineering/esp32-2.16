"""HTTP server for the Bridge.

Exposes `POST /talk`: the device sends captured audio, the Brain produces a
spoken reply, and the reply audio is returned. The Brain is injected so the
transport is decoupled from any particular Brain implementation.
"""

from __future__ import annotations

import logging

from flask import Flask, Response, jsonify, request

from orb_bridge.brain import Brain

REPLY_CONTENT_TYPE = "audio/wav"

log = logging.getLogger(__name__)


def create_app(brain: Brain) -> Flask:
    app = Flask(__name__)

    @app.post("/talk")
    def talk():
        audio = request.get_data()
        if not audio:
            return jsonify(error="no audio in request body"), 400

        try:
            reply = brain.respond(audio)
        except Exception as exc:  # noqa: BLE001 — surface, never swallow
            # Log the full context server-side; return a legible message to the device.
            log.exception("Brain failed to respond")
            return jsonify(error=f"brain failed: {exc}"), 502

        return Response(reply.reply_audio, mimetype=REPLY_CONTENT_TYPE)

    return app
