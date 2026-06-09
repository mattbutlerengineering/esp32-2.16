// Host test for the Motion app's read->map wiring, with a fake IMU seam.
// The real QMI8658 read and the rendering are hardware and live behind seams;
// this asserts that a tick samples the IMU and updates the bubble accordingly.

#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "motion_app.h"

static int g_checks = 0;
#define CHECK(cond) do { assert(cond); g_checks++; } while (0)
#define NEAR(a, b) (fabsf((a) - (b)) < 1e-4f)

// Fake IMU: returns a fixed gravity vector.
typedef struct { float ax, ay; } FakeImu;
static void fake_read(void *ctx, float *ax, float *ay) {
    FakeImu *f = ctx;
    *ax = f->ax;
    *ay = f->ay;
}

static void test_tick_maps_imu_to_bubble(void) {
    FakeImu imu = {.ax = 0.3f, .ay = 0.0f};
    MotionApp app;
    motion_app_init(&app, fake_read, &imu);

    motion_app_tick(&app);

    Bubble b = motion_app_bubble(&app);
    Bubble expected = tilt_bubble(0.3f, 0.0f);
    CHECK(NEAR(b.x, expected.x));
    CHECK(NEAR(b.y, expected.y));
    CHECK(b.x > 0.0f);
}

static void test_tick_tracks_changing_tilt(void) {
    FakeImu imu = {.ax = 0.0f, .ay = 0.0f};
    MotionApp app;
    motion_app_init(&app, fake_read, &imu);

    motion_app_tick(&app);
    CHECK(NEAR(motion_app_bubble(&app).x, 0.0f));  // level

    imu.ax = -0.4f;                                 // tilt left
    motion_app_tick(&app);
    CHECK(motion_app_bubble(&app).x < 0.0f);
}

int main(void) {
    test_tick_maps_imu_to_bubble();
    test_tick_tracks_changing_tilt();
    printf("motion_app: all %d checks passed\n", g_checks);
    return 0;
}
