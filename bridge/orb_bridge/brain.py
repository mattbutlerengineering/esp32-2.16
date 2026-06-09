"""The Brain: turns captured audio into a spoken reply.

`Brain` is the pluggable interface the Bridge depends on. `GeminiBrain` is the
default implementation; `ClaudeBrain` / `OllamaBrain` can be added later behind
the same interface without touching callers. The actual network call lives
behind a thin client seam (see `gemini_client.py`) so the orchestration logic
here is testable with a fake client.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Protocol


@dataclass(frozen=True)
class BrainReply:
    """What the Brain gives back: the words it chose and the audio to play."""

    reply_text: str
    reply_audio: bytes


class Brain(Protocol):
    def respond(self, audio: bytes) -> BrainReply:
        """Given captured speech audio, return a spoken reply."""
        ...


class GeminiClient(Protocol):
    """The narrow seam the GeminiBrain talks to. Real impl wraps google-genai."""

    def reply_to(self, audio: bytes) -> str:
        """Transcribe + reason over the audio, returning reply text."""
        ...

    def speak(self, text: str) -> bytes:
        """Synthesize speech audio for the given text."""
        ...


class GeminiBrain:
    """Default Brain: ask Gemini for a reply, then have it speak the reply."""

    def __init__(self, client: GeminiClient):
        self._client = client

    def respond(self, audio: bytes) -> BrainReply:
        reply_text = self._client.reply_to(audio)
        reply_audio = self._client.speak(reply_text)
        return BrainReply(reply_text=reply_text, reply_audio=reply_audio)
