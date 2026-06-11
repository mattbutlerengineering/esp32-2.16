// Host test for the ambient visualization kernel: pure pixel/field math plus
// the ripple + tilt state machine. No hardware — render targets a small
// in-memory RGB565 buffer.

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ambient.h"

static int g_checks = 0;
#define CHECK(cond) do { assert(cond); g_checks++; } while (0)
#define NEAR(a, b) (fabsf((a) - (b)) < 1e-4f)

static void test_init_zeroes_state(void) {
    Ambient a;
    memset(&a, 0xFF, sizeof a);  // poison so init must actually clear it
    ambient_init(&a);

    CHECK(NEAR(a.t, 0.0f));
    CHECK(NEAR(a.tilt_x, 0.0f));
    CHECK(NEAR(a.tilt_y, 0.0f));
    for (int i = 0; i < AMBIENT_RIPPLE_MAX; i++) {
        CHECK(!a.ripples[i].active);
    }
}

static void test_tick_advances_time(void) {
    Ambient a;
    ambient_init(&a);

    ambient_tick(&a, 0.033f, 0.0f, 0.0f);
    CHECK(NEAR(a.t, 0.033f));
    ambient_tick(&a, 0.033f, 0.0f, 0.0f);
    CHECK(NEAR(a.t, 0.066f));
}

static void test_tilt_smooths_toward_input(void) {
    Ambient a;
    ambient_init(&a);

    ambient_tick(&a, 0.033f, 1.0f, 0.0f);
    CHECK(a.tilt_x > 0.0f);     // moved toward the input...
    CHECK(a.tilt_x < 1.0f);     // ...but low-pass filtered, not a jump

    for (int i = 0; i < 300; i++) {
        ambient_tick(&a, 0.033f, 1.0f, 0.0f);
    }
    CHECK(a.tilt_x > 0.95f);    // converges
    CHECK(NEAR(a.tilt_y, 0.0f));
}

static void test_tilt_clamps_to_unit_range(void) {
    Ambient a;
    ambient_init(&a);

    for (int i = 0; i < 300; i++) {
        ambient_tick(&a, 0.033f, 5.0f, -5.0f);  // shaking the board
    }
    CHECK(a.tilt_x <= 1.0f);
    CHECK(a.tilt_y >= -1.0f);
}

static void test_touch_spawns_ripple(void) {
    Ambient a;
    ambient_init(&a);

    ambient_touch(&a, 0.25f, 0.75f);

    CHECK(a.ripples[0].active);
    CHECK(NEAR(a.ripples[0].cx, 0.25f));
    CHECK(NEAR(a.ripples[0].cy, 0.75f));
    CHECK(NEAR(a.ripples[0].age, 0.0f));
}

static void test_ripple_ages_and_expires(void) {
    Ambient a;
    ambient_init(&a);
    ambient_touch(&a, 0.5f, 0.5f);

    ambient_tick(&a, 0.1f, 0.0f, 0.0f);
    CHECK(a.ripples[0].active);
    CHECK(NEAR(a.ripples[0].age, 0.1f));

    for (int i = 0; i < (int)(AMBIENT_RIPPLE_LIFE / 0.1f) + 2; i++) {
        ambient_tick(&a, 0.1f, 0.0f, 0.0f);
    }
    CHECK(!a.ripples[0].active);  // lived its life, expired
}

static void test_touch_when_full_replaces_oldest(void) {
    Ambient a;
    ambient_init(&a);

    for (int i = 0; i < AMBIENT_RIPPLE_MAX; i++) {
        ambient_touch(&a, 0.1f * (float)i, 0.5f);
        ambient_tick(&a, 0.05f, 0.0f, 0.0f);  // stagger ages: earlier = older
    }
    ambient_touch(&a, 0.9f, 0.9f);  // registry full — should evict the oldest

    int active = 0;
    bool found_new = false, found_oldest = false;
    for (int i = 0; i < AMBIENT_RIPPLE_MAX; i++) {
        if (!a.ripples[i].active) continue;
        active++;
        if (NEAR(a.ripples[i].cx, 0.9f)) found_new = true;
        if (NEAR(a.ripples[i].cx, 0.0f)) found_oldest = true;
    }
    CHECK(active == AMBIENT_RIPPLE_MAX);
    CHECK(found_new);
    CHECK(!found_oldest);
}

enum { W = 16, H = 16, N = W * H };

// Render is deterministic and writes every pixel: two buffers with different
// poison values must agree exactly after rendering the same state.
static void test_render_writes_every_pixel(void) {
    Ambient a;
    ambient_init(&a);
    ambient_tick(&a, 0.5f, 0.0f, 0.0f);

    uint16_t fb1[N], fb2[N];
    memset(fb1, 0xAA, sizeof fb1);
    memset(fb2, 0x55, sizeof fb2);
    ambient_render(&a, fb1, W, H);
    ambient_render(&a, fb2, W, H);

    CHECK(memcmp(fb1, fb2, sizeof fb1) == 0);
}

static int frames_differ(const uint16_t *x, const uint16_t *y) {
    return memcmp(x, y, N * sizeof(uint16_t)) != 0;
}

static void test_render_animates_over_time(void) {
    Ambient a;
    ambient_init(&a);

    uint16_t before[N], after[N];
    ambient_render(&a, before, W, H);
    ambient_tick(&a, 1.0f, 0.0f, 0.0f);
    ambient_render(&a, after, W, H);

    CHECK(frames_differ(before, after));
}

static void test_ripple_changes_the_frame(void) {
    Ambient a;
    ambient_init(&a);
    ambient_tick(&a, 0.5f, 0.0f, 0.0f);

    uint16_t calm[N], rippled[N];
    ambient_render(&a, calm, W, H);
    ambient_touch(&a, 0.5f, 0.5f);
    ambient_tick(&a, 0.1f, 0.0f, 0.0f);

    Ambient calm_state;
    ambient_init(&calm_state);
    ambient_tick(&calm_state, 0.5f, 0.0f, 0.0f);
    ambient_tick(&calm_state, 0.1f, 0.0f, 0.0f);
    uint16_t calm2[N];
    ambient_render(&calm_state, calm2, W, H);

    ambient_render(&a, rippled, W, H);
    CHECK(frames_differ(rippled, calm2));
    (void)calm;
}

static void test_tilt_warps_the_field(void) {
    Ambient flat, tilted;
    ambient_init(&flat);
    ambient_init(&tilted);
    ambient_tick(&flat, 0.5f, 0.0f, 0.0f);
    for (int i = 0; i < 100; i++) {
        ambient_tick(&tilted, 0.005f, 1.0f, 0.0f);  // same total t, full tilt
    }

    uint16_t fb_flat[N], fb_tilted[N];
    ambient_render(&flat, fb_flat, W, H);
    ambient_render(&tilted, fb_tilted, W, H);

    CHECK(frames_differ(fb_flat, fb_tilted));
}

int main(void) {
    test_init_zeroes_state();
    test_tick_advances_time();
    test_tilt_smooths_toward_input();
    test_tilt_clamps_to_unit_range();
    test_touch_spawns_ripple();
    test_ripple_ages_and_expires();
    test_touch_when_full_replaces_oldest();
    test_render_writes_every_pixel();
    test_render_animates_over_time();
    test_ripple_changes_the_frame();
    test_tilt_warps_the_field();
    printf("ambient: all %d checks passed\n", g_checks);
    return 0;
}
