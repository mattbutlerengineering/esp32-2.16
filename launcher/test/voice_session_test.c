// Host unit tests for the voice-session coordinator — the orchestration that
// drives the Orb state machine through a tap-to-talk interaction. The record,
// send-to-Bridge, and play steps are injected seams, so the whole flow is
// host-testable with fakes; the real I2S capture / HTTP / playback plug into the
// same seams on the device.

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "voice_session.h"

static int g_checks = 0;
#define CHECK(cond) do { assert(cond); g_checks++; } while (0)

// A fake that records what it was asked to do and lets the test choose whether
// the Bridge succeeds.
typedef struct {
    bool send_ok;
    Audio captured_seen;   // what send() received
    Audio played;          // what play() received
    bool played_called;
    int record_calls;
} Fake;

static Audio fake_record(void *ctx) {
    Fake *f = ctx;
    f->record_calls++;
    Audio a = {(const uint8_t *)"MIC", 3};
    return a;
}
static BridgeReply fake_send(void *ctx, Audio captured) {
    Fake *f = ctx;
    f->captured_seen = captured;
    BridgeReply r;
    r.ok = f->send_ok;
    Audio reply = {(const uint8_t *)"REPLY", 5};
    r.audio = reply;
    return r;
}
static void fake_play(void *ctx, Audio reply) {
    Fake *f = ctx;
    f->played = reply;
    f->played_called = true;
}

static VoiceSession make_session(Fake *f) {
    VoiceSession vs;
    voice_session_init(&vs, fake_record, fake_send, fake_play, f);
    return vs;
}

// Tapping puts the session into listening.
static void test_tap_listens(void) {
    Fake f = {.send_ok = true};
    VoiceSession vs = make_session(&f);

    voice_session_tap(&vs);

    CHECK(voice_session_state(&vs) == ORB_LISTENING);
}

// Happy path: tap -> release captures, sends the recording, plays the reply,
// and the Orb returns to idle.
static void test_full_interaction_success(void) {
    Fake f = {.send_ok = true};
    VoiceSession vs = make_session(&f);

    voice_session_tap(&vs);
    OrbState end = voice_session_release(&vs);

    CHECK(end == ORB_IDLE);
    CHECK(f.record_calls == 1);
    CHECK(memcmp(f.captured_seen.data, "MIC", 3) == 0);  // sent what was recorded
    CHECK(f.played_called);
    CHECK(memcmp(f.played.data, "REPLY", 5) == 0);        // played the Bridge reply
}

// Bridge failure drops the Orb to error and never plays anything.
static void test_bridge_failure_goes_to_error(void) {
    Fake f = {.send_ok = false};
    VoiceSession vs = make_session(&f);

    voice_session_tap(&vs);
    OrbState end = voice_session_release(&vs);

    CHECK(end == ORB_ERROR);
    CHECK(!f.played_called);
}

int main(void) {
    test_tap_listens();
    test_full_interaction_success();
    test_bridge_failure_goes_to_error();
    printf("voice_session: all %d checks passed\n", g_checks);
    return 0;
}
