// Voice-session coordinator: orchestrates one tap-to-talk interaction and drives
// the Orb state machine through it.
//
// The three side-effecting steps — capture audio, send it to the Bridge, play
// the reply — are injected as seams. That keeps the orchestration (the order of
// steps and the FSM transitions, including the failure path) host-testable. On
// the device, the real I2S capture, HTTP-to-Bridge, and I2S playback plug into
// the same seams.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "orb_fsm.h"

typedef struct {
    const uint8_t *data;
    size_t len;
} Audio;

typedef struct {
    bool ok;       // did the Bridge/Brain succeed?
    Audio audio;   // reply audio to play (valid when ok)
} BridgeReply;

typedef Audio (*RecordFn)(void *ctx);                 // capture mic audio
typedef BridgeReply (*SendFn)(void *ctx, Audio captured); // POST to the Bridge
typedef void (*PlayFn)(void *ctx, Audio reply);       // play reply audio

typedef struct {
    RecordFn record;
    SendFn send;
    PlayFn play;
    void *ctx;
    OrbFsm fsm;
} VoiceSession;

void voice_session_init(VoiceSession *vs, RecordFn record, SendFn send,
                        PlayFn play, void *ctx);

// Tap-to-talk pressed: begin listening.
void voice_session_tap(VoiceSession *vs);

// Tap-to-talk released: capture -> send -> (on success) play, driving the Orb
// through thinking -> speaking -> idle, or to error on Bridge failure. Returns
// the resulting Orb state.
OrbState voice_session_release(VoiceSession *vs);

OrbState voice_session_state(const VoiceSession *vs);
