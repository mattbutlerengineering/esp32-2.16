"""Real Gemini client seam for `GeminiBrain` (free-tier Gemini Developer API).

This is the one piece that actually hits the network, so it is deliberately
thin and kept out of the unit suite — `GeminiBrain`'s orchestration is tested
against a fake implementing the same two-method seam. Verify this against live
Gemini with a real `GEMINI_API_KEY`.

Free tier: get a key at https://aistudio.google.com/apikey and export it as
`GEMINI_API_KEY`.
"""

from __future__ import annotations

import io
import os
import wave

from google import genai
from google.genai import types

# Audio-native model for understanding the captured speech, and a TTS model for
# speaking the reply. Both are available on the free Gemini Developer API.
REPLY_MODEL = "gemini-2.5-flash"
TTS_MODEL = "gemini-2.5-flash-preview-tts"
TTS_VOICE = "Kore"

# Gemini TTS returns raw little-endian 16-bit PCM at 24 kHz mono.
_TTS_SAMPLE_RATE = 24_000

SYSTEM_PROMPT = (
    "You are Orb, a friendly voice companion on a small round display. "
    "The user spoke to you; reply in one or two short, natural spoken sentences."
)


class GeminiClient:
    """Wraps google-genai into the narrow seam `GeminiBrain` depends on."""

    def __init__(self, api_key: str | None = None, *, request_mime_type: str = "audio/wav"):
        key = api_key or os.environ.get("GEMINI_API_KEY")
        if not key:
            raise RuntimeError(
                "GEMINI_API_KEY is not set. Get a free key at "
                "https://aistudio.google.com/apikey and export GEMINI_API_KEY."
            )
        self._client = genai.Client(api_key=key)
        self._request_mime_type = request_mime_type

    def reply_to(self, audio: bytes) -> str:
        """Send the captured audio to Gemini and return the reply text."""
        response = self._client.models.generate_content(
            model=REPLY_MODEL,
            contents=[
                SYSTEM_PROMPT,
                types.Part.from_bytes(data=audio, mime_type=self._request_mime_type),
            ],
        )
        text = (response.text or "").strip()
        if not text:
            raise RuntimeError("Gemini returned no reply text")
        return text

    def speak(self, text: str) -> bytes:
        """Synthesize the reply text to a WAV byte string the device can play."""
        response = self._client.models.generate_content(
            model=TTS_MODEL,
            contents=text,
            config=types.GenerateContentConfig(
                response_modalities=["AUDIO"],
                speech_config=types.SpeechConfig(
                    voice_config=types.VoiceConfig(
                        prebuilt_voice_config=types.PrebuiltVoiceConfig(
                            voice_name=TTS_VOICE
                        )
                    )
                ),
            ),
        )
        pcm = response.candidates[0].content.parts[0].inline_data.data
        if not pcm:
            raise RuntimeError("Gemini TTS returned no audio")
        return _pcm_to_wav(pcm, _TTS_SAMPLE_RATE)


def _pcm_to_wav(pcm: bytes, sample_rate: int) -> bytes:
    """Wrap raw 16-bit mono PCM in a WAV container."""
    buf = io.BytesIO()
    with wave.open(buf, "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)  # 16-bit
        wav.setframerate(sample_rate)
        wav.writeframes(pcm)
    return buf.getvalue()
