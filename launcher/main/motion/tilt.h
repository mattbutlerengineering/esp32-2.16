// Tilt -> bubble mapping for the Motion Mini-app (a bubble level / rolling ball).
//
// Pure math: given the gravity vector's screen-plane components (in g, as read
// from the QMI8658 accelerometer), produce a normalized bubble offset within the
// unit disc. Hardware-free and host-unit-testable.
#pragma once

typedef struct {
    float x;  // -1..1, +x = screen right
    float y;  // -1..1, +y = screen down
} Bubble;

// Map gravity components (ax, ay) in g to a bubble offset. The ball rolls in the
// direction of tilt; the offset magnitude is clamped to the unit disc (radius 1).
Bubble tilt_bubble(float ax, float ay);
