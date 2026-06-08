"""Behavior: POST /talk relays the device's audio through the Brain and
returns the reply audio. The Brain is faked — these tests assert the HTTP
contract, not any specific Brain."""

import pytest

from orb_bridge.brain import BrainReply
from orb_bridge.server import create_app


class FakeBrain:
    def __init__(self, reply=BrainReply("hi", b"REPLY_AUDIO"), error=None):
        self._reply = reply
        self._error = error
        self.heard = None

    def respond(self, audio):
        self.heard = audio
        if self._error is not None:
            raise self._error
        return self._reply


@pytest.fixture
def client_and_brain():
    brain = FakeBrain()
    app = create_app(brain)
    return app.test_client(), brain


def test_talk_returns_brain_reply_audio(client_and_brain):
    client, brain = client_and_brain

    resp = client.post("/talk", data=b"CAPTURED_SPEECH",
                       content_type="application/octet-stream")

    assert resp.status_code == 200
    assert resp.data == b"REPLY_AUDIO"
    # The captured audio actually reached the Brain.
    assert brain.heard == b"CAPTURED_SPEECH"


def test_talk_surfaces_brain_failure_legibly():
    # The Brain (e.g. Gemini/network) blows up.
    brain = FakeBrain(error=RuntimeError("gemini timed out"))
    client = create_app(brain).test_client()

    resp = client.post("/talk", data=b"CAPTURED_SPEECH",
                       content_type="application/octet-stream")

    # Not a silent success: an upstream-failure status with a readable message.
    assert resp.status_code == 502
    assert resp.is_json
    assert "gemini timed out" in resp.get_json()["error"]


def test_talk_rejects_empty_audio():
    brain = FakeBrain()
    client = create_app(brain).test_client()

    resp = client.post("/talk", data=b"", content_type="application/octet-stream")

    assert resp.status_code == 400
    assert resp.is_json
    # The Brain should never have been called with empty input.
    assert brain.heard is None
