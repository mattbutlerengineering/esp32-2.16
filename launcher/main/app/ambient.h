// Ambient visualization kernel: a tilt-warped plasma field with touch ripples
// and a breathing glow, rendered into an RGB565 framebuffer.
//
// Pure math + state — no hardware, no ESP-IDF. The Mini-app glue feeds it
// touch/IMU samples and a framebuffer; everything here is host-unit-testable.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define AMBIENT_RIPPLE_MAX  3      // concurrent touch ripples
#define AMBIENT_RIPPLE_LIFE 1.6f   // seconds a ripple lives

typedef struct {
    float cx, cy;  // origin, normalized 0..1 screen coords
    float age;     // seconds since the touch
    bool active;
} AmbientRipple;

typedef struct {
    float t;               // animation time, seconds
    float tilt_x, tilt_y;  // low-pass-filtered tilt, clamped to -1..1
    AmbientRipple ripples[AMBIENT_RIPPLE_MAX];
} Ambient;

void ambient_init(Ambient *a);

// Advance one frame: move time forward by `dt` seconds, smooth the tilt toward
// the raw accelerometer reading (ax, ay in g), and age/expire ripples.
void ambient_tick(Ambient *a, float dt, float ax, float ay);

// Register a touch at normalized (0..1) screen coords: spawns a ripple,
// evicting the oldest one if all slots are taken.
void ambient_touch(Ambient *a, float x, float y);

// Render the current state into an RGB565 buffer of w x h pixels (panel byte
// order, same as the display driver's). Deterministic: same state, same frame.
void ambient_render(const Ambient *a, uint16_t *fb, int w, int h);
