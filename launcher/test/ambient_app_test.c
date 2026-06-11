// Host test for the Ambient app's seam wiring: touch edge detection (a held
// finger spawns one ripple, not one per frame) and IMU -> tilt feed. The real
// CST9217/QMI8658 reads are hardware and live behind the injected seams.

#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "ambient_app.h"

static int g_checks = 0;
#define CHECK(cond) do { assert(cond); g_checks++; } while (0)
#define NEAR(a, b) (fabsf((a) - (b)) < 1e-3f)

typedef struct {
    bool down;
    uint16_t x, y;
} FakeTouch;

static bool fake_touch(void *ctx, uint16_t *x, uint16_t *y) {
    FakeTouch *t = ctx;
    if (t->down) {
        *x = t->x;
        *y = t->y;
    }
    return t->down;
}

typedef struct { float ax, ay; } FakeImu;

static void fake_imu(void *ctx, float *ax, float *ay) {
    FakeImu *f = ctx;
    *ax = f->ax;
    *ay = f->ay;
}

static int active_ripples(const AmbientApp *app) {
    const Ambient *a = ambient_app_ambient(app);
    int n = 0;
    for (int i = 0; i < AMBIENT_RIPPLE_MAX; i++) {
        n += a->ripples[i].active ? 1 : 0;
    }
    return n;
}

static void test_tick_advances_kernel_time(void) {
    FakeTouch touch = {0};
    FakeImu imu = {0};
    AmbientApp app;
    ambient_app_init(&app, fake_touch, &touch, fake_imu, &imu, 480, 480);

    ambient_app_tick(&app, 0.033f);
    CHECK(NEAR(ambient_app_ambient(&app)->t, 0.033f));
}

static void test_new_contact_spawns_one_normalized_ripple(void) {
    FakeTouch touch = {.down = true, .x = 240, .y = 120};
    FakeImu imu = {0};
    AmbientApp app;
    ambient_app_init(&app, fake_touch, &touch, fake_imu, &imu, 480, 480);

    ambient_app_tick(&app, 0.033f);

    const Ambient *a = ambient_app_ambient(&app);
    CHECK(active_ripples(&app) == 1);
    CHECK(NEAR(a->ripples[0].cx, 240.0f / 479.0f));
    CHECK(NEAR(a->ripples[0].cy, 120.0f / 479.0f));
}

static void test_held_finger_does_not_respawn(void) {
    FakeTouch touch = {.down = true, .x = 100, .y = 100};
    FakeImu imu = {0};
    AmbientApp app;
    ambient_app_init(&app, fake_touch, &touch, fake_imu, &imu, 480, 480);

    for (int i = 0; i < 10; i++) {
        ambient_app_tick(&app, 0.033f);  // finger stays down
    }
    CHECK(active_ripples(&app) == 1);

    touch.down = false;
    ambient_app_tick(&app, 0.033f);  // lift...
    touch.down = true;
    ambient_app_tick(&app, 0.033f);  // ...and tap again
    CHECK(active_ripples(&app) == 2);
}

static void test_imu_feeds_tilt(void) {
    FakeTouch touch = {0};
    FakeImu imu = {.ax = 0.8f, .ay = -0.4f};
    AmbientApp app;
    ambient_app_init(&app, fake_touch, &touch, fake_imu, &imu, 480, 480);

    for (int i = 0; i < 300; i++) {
        ambient_app_tick(&app, 0.033f);
    }
    const Ambient *a = ambient_app_ambient(&app);
    CHECK(a->tilt_x > 0.7f);
    CHECK(a->tilt_y < -0.3f);
}

int main(void) {
    test_tick_advances_kernel_time();
    test_new_contact_spawns_one_normalized_ripple();
    test_held_finger_does_not_respawn();
    test_imu_feeds_tilt();
    printf("ambient_app: all %d checks passed\n", g_checks);
    return 0;
}
