// Host unit tests for the tilt -> bubble mapping (Motion Mini-app kernel).
// Pure math, no hardware. The IMU read and rendering live elsewhere; this
// asserts only the mapping from a gravity vector to a clamped screen offset.

#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "tilt.h"

static int g_checks = 0;
#define CHECK(cond) do { assert(cond); g_checks++; } while (0)
#define NEAR(a, b) (fabsf((a) - (b)) < 1e-4f)

// Board flat (no tilt) -> bubble centered.
static void test_level_is_centered(void) {
    Bubble b = tilt_bubble(0.0f, 0.0f);
    CHECK(NEAR(b.x, 0.0f));
    CHECK(NEAR(b.y, 0.0f));
}

// Tilt along +x moves the bubble in +x only.
static void test_tilt_x_moves_x(void) {
    Bubble b = tilt_bubble(0.3f, 0.0f);
    CHECK(b.x > 0.0f);
    CHECK(NEAR(b.y, 0.0f));
}

// A large tilt clamps the bubble to the unit disc (magnitude 1), not beyond.
static void test_extreme_tilt_clamps(void) {
    Bubble b = tilt_bubble(1.0f, 0.0f);  // 1g sideways -> well past full scale
    float mag = sqrtf(b.x * b.x + b.y * b.y);
    CHECK(NEAR(mag, 1.0f));
    CHECK(b.x > 0.0f);
}

// Diagonal tilt keeps a 45-degree direction (x == y) even after clamping.
static void test_diagonal_direction_preserved(void) {
    Bubble b = tilt_bubble(0.8f, 0.8f);
    CHECK(NEAR(b.x, b.y));
    CHECK(b.x > 0.0f);
}

// Sign follows the tilt: -x tilt -> bubble to the left.
static void test_negative_tilt(void) {
    Bubble b = tilt_bubble(-0.3f, 0.0f);
    CHECK(b.x < 0.0f);
}

int main(void) {
    test_level_is_centered();
    test_tilt_x_moves_x();
    test_extreme_tilt_clamps();
    test_diagonal_direction_preserved();
    test_negative_tilt();
    printf("tilt: all %d checks passed\n", g_checks);
    return 0;
}
