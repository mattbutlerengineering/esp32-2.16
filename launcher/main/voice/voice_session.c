#include "voice_session.h"

void voice_session_init(VoiceSession *vs, RecordFn record, SendFn send,
                        PlayFn play, void *ctx) {
    vs->record = record;
    vs->send = send;
    vs->play = play;
    vs->ctx = ctx;
    orb_fsm_init(&vs->fsm);
}

void voice_session_tap(VoiceSession *vs) {
    orb_fsm_handle(&vs->fsm, ORB_EV_TAP);  // -> LISTENING
}

OrbState voice_session_release(VoiceSession *vs) {
    Audio captured = vs->record(vs->ctx);
    orb_fsm_handle(&vs->fsm, ORB_EV_SEND);  // -> THINKING

    BridgeReply reply = vs->send(vs->ctx, captured);
    if (!reply.ok) {
        return orb_fsm_handle(&vs->fsm, ORB_EV_FAILURE);  // -> ERROR
    }

    orb_fsm_handle(&vs->fsm, ORB_EV_REPLY_READY);  // -> SPEAKING
    vs->play(vs->ctx, reply.audio);
    return orb_fsm_handle(&vs->fsm, ORB_EV_PLAYBACK_DONE);  // -> IDLE
}

OrbState voice_session_state(const VoiceSession *vs) {
    return orb_fsm_state(&vs->fsm);
}
