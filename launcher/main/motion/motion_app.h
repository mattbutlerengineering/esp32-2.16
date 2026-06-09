// Motion Mini-app: a bubble level driven by the IMU.
//
// The IMU read is injected as a seam so the read->map wiring is host-testable
// with a fake. The real QMI8658 driver and the on-screen rendering are hardware
// and plug into this seam / the bubble position.
#pragma once

#include "tilt.h"

// Reads the gravity vector's screen-plane components (in g) into *ax,*ay.
typedef void (*ImuReadFn)(void *ctx, float *ax, float *ay);

typedef struct {
    ImuReadFn read;
    void *ctx;
    Bubble bubble;  // latest mapped bubble position
} MotionApp;

void motion_app_init(MotionApp *app, ImuReadFn read, void *ctx);

// Sample the IMU and update the bubble. Call each frame while active.
void motion_app_tick(MotionApp *app);

// Latest bubble position (what the renderer draws).
Bubble motion_app_bubble(const MotionApp *app);
