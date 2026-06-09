#include "tilt.h"

#include <math.h>

// Gain maps a modest tilt to a full-scale bubble swing; ~1g of lateral gravity
// pushes the bubble well past the edge, then it clamps.
#define TILT_GAIN 2.0f

Bubble tilt_bubble(float ax, float ay) {
    float x = ax * TILT_GAIN;
    float y = ay * TILT_GAIN;

    // Clamp to the unit disc, preserving direction.
    float mag = sqrtf(x * x + y * y);
    if (mag > 1.0f) {
        x /= mag;
        y /= mag;
    }

    Bubble b = {x, y};
    return b;
}
