"""Behavior: a Brain turns captured audio into a spoken reply.

The network call to Gemini is isolated behind a client seam, so these tests
run with no API key and no network — they assert the Brain's behavior, not
the SDK's wire format.
"""

from orb_bridge.brain import BrainReply, GeminiBrain


class FakeGeminiClient:
    """Stand-in for the real Gemini client seam."""

    def __init__(self, reply_text="hello there", reply_audio=b"AUDIO"):
        self._reply_text = reply_text
        self._reply_audio = reply_audio
        self.heard = None

    def reply_to(self, audio):
        self.heard = audio
        return self._reply_text

    def speak(self, text):
        assert text == self._reply_text
        return self._reply_audio


def test_brain_returns_spoken_reply_for_audio():
    client = FakeGeminiClient(reply_text="the time is noon", reply_audio=b"SPOKEN")
    brain = GeminiBrain(client)

    reply = brain.respond(b"WHAT_TIME_IS_IT")

    assert isinstance(reply, BrainReply)
    assert reply.reply_text == "the time is noon"
    assert reply.reply_audio == b"SPOKEN"
    # The Brain actually fed the captured audio to the client.
    assert client.heard == b"WHAT_TIME_IS_IT"
