// Ambient Mini-app: glues the ambient kernel to the board.
//
// Touch and IMU reads are injected as seams (same pattern as the Motion app)
// so the poll -> edge-detect -> kernel wiring is host-testable with fakes.
// The real CST9217/QMI8658 reads and the framebuffer render plug in from the
// Launcher.
#pragma once

#include "ambient.h"

#include <stdint.h>

// Polls the touch panel once; returns true with pixel coords while a finger
// is down (the CST9217 driver's touch_poll signature, plus a ctx).
typedef bool (*AmbientTouchFn)(void *ctx, uint16_t *x, uint16_t *y);

// Reads the gravity vector's screen-plane components (in g) into *ax,*ay.
typedef void (*AmbientImuFn)(void *ctx, float *ax, float *ay);

typedef struct {
    AmbientTouchFn touch;
    void *touch_ctx;
    AmbientImuFn imu;
    void *imu_ctx;
    int w, h;           // screen size, for normalizing touch coords
    bool was_touched;   // previous frame's contact, for tap edge detection
    Ambient ambient;
} AmbientApp;

void ambient_app_init(AmbientApp *app, AmbientTouchFn touch, void *touch_ctx,
                      AmbientImuFn imu, void *imu_ctx, int w, int h);

// One frame: poll touch (a new contact spawns a ripple; a held finger does
// not), sample the IMU, and advance the kernel by `dt` seconds.
void ambient_app_tick(AmbientApp *app, float dt);

// The kernel state (what the renderer draws).
const Ambient *ambient_app_ambient(const AmbientApp *app);
