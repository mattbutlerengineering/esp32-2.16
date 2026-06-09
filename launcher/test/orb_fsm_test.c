// Host unit tests for the Orb state machine — pure logic, no hardware, no
// ESP-IDF. Compile and run with `make` in this directory (see Makefile).
//
// Tests assert observable behavior: which state a given event sequence lands in,
// not internal representation.

#include <assert.h>
#include <stdio.h>

#include "orb_fsm.h"

static int g_checks = 0;
#define CHECK(cond) do { assert(cond); g_checks++; } while (0)

// A tap in IDLE starts listening (tap-to-talk).
static void test_tap_starts_listening(void) {
    OrbFsm fsm;
    orb_fsm_init(&fsm);
    CHECK(orb_fsm_state(&fsm) == ORB_IDLE);

    orb_fsm_handle(&fsm, ORB_EV_TAP);
    CHECK(orb_fsm_state(&fsm) == ORB_LISTENING);
}

// The full happy path: idle -> listening -> thinking -> speaking -> idle.
static void test_happy_path(void) {
    OrbFsm fsm;
    orb_fsm_init(&fsm);

    CHECK(orb_fsm_handle(&fsm, ORB_EV_TAP) == ORB_LISTENING);
    CHECK(orb_fsm_handle(&fsm, ORB_EV_SEND) == ORB_THINKING);
    CHECK(orb_fsm_handle(&fsm, ORB_EV_REPLY_READY) == ORB_SPEAKING);
    CHECK(orb_fsm_handle(&fsm, ORB_EV_PLAYBACK_DONE) == ORB_IDLE);
}

// A failure at any active stage drops to ERROR.
static void test_failure_from_each_active_state(void) {
    OrbFsm fsm;

    orb_fsm_init(&fsm);
    orb_fsm_handle(&fsm, ORB_EV_TAP);                 // LISTENING
    CHECK(orb_fsm_handle(&fsm, ORB_EV_FAILURE) == ORB_ERROR);

    orb_fsm_init(&fsm);
    orb_fsm_handle(&fsm, ORB_EV_TAP);
    orb_fsm_handle(&fsm, ORB_EV_SEND);                // THINKING
    CHECK(orb_fsm_handle(&fsm, ORB_EV_FAILURE) == ORB_ERROR);

    orb_fsm_init(&fsm);
    orb_fsm_handle(&fsm, ORB_EV_TAP);
    orb_fsm_handle(&fsm, ORB_EV_SEND);
    orb_fsm_handle(&fsm, ORB_EV_REPLY_READY);         // SPEAKING
    CHECK(orb_fsm_handle(&fsm, ORB_EV_FAILURE) == ORB_ERROR);
}

// From ERROR: retry resumes listening, dismiss returns to rest.
static void test_error_recovery(void) {
    OrbFsm fsm;
    orb_fsm_init(&fsm);
    orb_fsm_handle(&fsm, ORB_EV_TAP);
    orb_fsm_handle(&fsm, ORB_EV_FAILURE);             // ERROR
    CHECK(orb_fsm_handle(&fsm, ORB_EV_RETRY) == ORB_LISTENING);

    orb_fsm_handle(&fsm, ORB_EV_FAILURE);             // ERROR again
    CHECK(orb_fsm_handle(&fsm, ORB_EV_DISMISS) == ORB_IDLE);
}

// Cancelling while listening returns to rest without sending.
static void test_cancel_listening(void) {
    OrbFsm fsm;
    orb_fsm_init(&fsm);
    orb_fsm_handle(&fsm, ORB_EV_TAP);
    CHECK(orb_fsm_handle(&fsm, ORB_EV_CANCEL) == ORB_IDLE);
}

// Events invalid for the current state are no-ops.
static void test_invalid_events_are_noops(void) {
    OrbFsm fsm;
    orb_fsm_init(&fsm);
    CHECK(orb_fsm_handle(&fsm, ORB_EV_REPLY_READY) == ORB_IDLE);
    CHECK(orb_fsm_handle(&fsm, ORB_EV_PLAYBACK_DONE) == ORB_IDLE);

    orb_fsm_handle(&fsm, ORB_EV_TAP);                 // LISTENING
    CHECK(orb_fsm_handle(&fsm, ORB_EV_PLAYBACK_DONE) == ORB_LISTENING);
}

int main(void) {
    test_tap_starts_listening();
    test_happy_path();
    test_failure_from_each_active_state();
    test_error_recovery();
    test_cancel_listening();
    test_invalid_events_are_noops();
    printf("orb_fsm: all %d checks passed\n", g_checks);
    return 0;
}
