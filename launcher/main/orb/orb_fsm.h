// Orb state machine — the explicit lifecycle of the Voice assistant's presence.
//
// Pure logic, no hardware dependencies, so it is host-unit-testable. The UI maps
// each state to a distinct Orb visual; the audio/network layers feed it events.
#pragma once

typedef enum {
    ORB_IDLE = 0,    // resting / ambient animation
    ORB_LISTENING,   // capturing the user's speech
    ORB_THINKING,    // waiting on the Brain's reply
    ORB_SPEAKING,    // playing the reply
    ORB_ERROR,       // bridge/network failure; awaiting dismiss/retry
} OrbState;

typedef enum {
    ORB_EV_TAP = 0,     // tap-to-talk: begin listening
    ORB_EV_SEND,        // user finished; audio sent to the Bridge
    ORB_EV_CANCEL,      // abort listening without sending
    ORB_EV_REPLY_READY, // the Brain's reply audio arrived
    ORB_EV_PLAYBACK_DONE,// finished playing the reply
    ORB_EV_FAILURE,     // bridge/WiFi/Brain failure
    ORB_EV_RETRY,       // from error: try again
    ORB_EV_DISMISS,     // from error: return to rest
} OrbEvent;

typedef struct {
    OrbState state;
} OrbFsm;

// Initialize to ORB_IDLE.
void orb_fsm_init(OrbFsm *fsm);

// Current state.
OrbState orb_fsm_state(const OrbFsm *fsm);

// Apply an event. Returns the resulting state. Events that are not valid for the
// current state leave the state unchanged (no-op).
OrbState orb_fsm_handle(OrbFsm *fsm, OrbEvent event);
