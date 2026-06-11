// Ambient visualization kernel. All per-pixel work is integer + lookup-table
// (the 480x480 panel is 230k pixels at 30 FPS — float per pixel won't fit the
// frame budget on the ESP32-S3). Field values are computed once per 2x2 block.

#include "ambient.h"

#include <math.h>

#define TILT_TAU      0.15f  // tilt low-pass time constant, seconds
#define RIPPLE_REACH  200    // ripple influence radius, 0..255 screen units

static int8_t s_sin[256];     // sin LUT, amplitude 127
static uint16_t s_pal[256];   // colour LUT, panel-order RGB565
static bool s_tables_built = false;

static uint16_t pack565(int r, int g, int b) {
    uint16_t c = (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
    return (uint16_t)((c << 8) | (c >> 8));  // panel is big-endian per pixel
}

// Deep-space gradient: near-black navy -> ocean blue -> cyan -> white-hot.
static void build_tables(void) {
    static const struct { int r, g, b; } stops[4] = {
        {2, 4, 18}, {0, 70, 120}, {40, 210, 215}, {250, 240, 255},
    };
    for (int i = 0; i < 256; i++) {
        s_sin[i] = (int8_t)(127.0f * sinf((float)i * (float)(2.0 * M_PI / 256.0)));
        int seg = (i * 3) >> 8;          // gradient segment 0..2
        int f = (i * 3) & 255;           // position within the segment
        s_pal[i] = pack565(
            stops[seg].r + ((stops[seg + 1].r - stops[seg].r) * f >> 8),
            stops[seg].g + ((stops[seg + 1].g - stops[seg].g) * f >> 8),
            stops[seg].b + ((stops[seg + 1].b - stops[seg].b) * f >> 8));
    }
    s_tables_built = true;
}

static float clamp_unit(float v) {
    return v < -1.0f ? -1.0f : v > 1.0f ? 1.0f : v;
}

void ambient_init(Ambient *a) {
    a->t = 0.0f;
    a->tilt_x = 0.0f;
    a->tilt_y = 0.0f;
    for (int i = 0; i < AMBIENT_RIPPLE_MAX; i++) {
        a->ripples[i] = (AmbientRipple){0};
    }
}

void ambient_tick(Ambient *a, float dt, float ax, float ay) {
    a->t += dt;

    const float k = 1.0f - expf(-dt / TILT_TAU);
    a->tilt_x += (clamp_unit(ax) - a->tilt_x) * k;
    a->tilt_y += (clamp_unit(ay) - a->tilt_y) * k;

    for (int i = 0; i < AMBIENT_RIPPLE_MAX; i++) {
        if (!a->ripples[i].active) continue;
        a->ripples[i].age += dt;
        if (a->ripples[i].age >= AMBIENT_RIPPLE_LIFE) {
            a->ripples[i].active = false;
        }
    }
}

void ambient_touch(Ambient *a, float x, float y) {
    int slot = -1;
    float oldest = -1.0f;
    for (int i = 0; i < AMBIENT_RIPPLE_MAX; i++) {
        if (!a->ripples[i].active) {
            slot = i;
            break;
        }
        if (a->ripples[i].age > oldest) {
            oldest = a->ripples[i].age;
            slot = i;  // all full: evict the oldest
        }
    }
    a->ripples[slot] = (AmbientRipple){.cx = x, .cy = y, .age = 0.0f, .active = true};
}

// Integer per-frame view of the state: phases, tilt-shifted glow centre, and
// ripple params, all in 0..255 normalized screen units.
typedef struct {
    int t1, t2, t3;  // wave phases
    int breath;      // global brightness pulse, -127..127
    int cx, cy;      // glow centre
    struct { int x, y, phase, env; } rip[AMBIENT_RIPPLE_MAX];
    int nrip;
} Frame;

static Frame frame_of(const Ambient *a) {
    Frame f;
    f.t1 = (int)(a->t * 40.0f);
    f.t2 = (int)(a->t * 53.0f);
    f.t3 = (int)(a->t * 27.0f);
    f.breath = s_sin[(int)(a->t * 18.0f) & 255];
    f.cx = 128 + (int)(a->tilt_x * 48.0f);
    f.cy = 128 + (int)(a->tilt_y * 48.0f);
    f.nrip = 0;
    for (int i = 0; i < AMBIENT_RIPPLE_MAX; i++) {
        if (!a->ripples[i].active) continue;
        const float age = a->ripples[i].age;
        f.rip[f.nrip].x = (int)(a->ripples[i].cx * 255.0f);
        f.rip[f.nrip].y = (int)(a->ripples[i].cy * 255.0f);
        f.rip[f.nrip].phase = (int)(age * 340.0f);
        f.rip[f.nrip].env = (int)(127.0f * (1.0f - age / AMBIENT_RIPPLE_LIFE));
        f.nrip++;
    }
    return f;
}

// Octagonal distance: max + min/2, a sqrt-free approximation good enough for
// organic-looking rings.
static inline int oct_dist(int dx, int dy) {
    int adx = dx < 0 ? -dx : dx;
    int ady = dy < 0 ? -dy : dy;
    return adx > ady ? adx + (ady >> 1) : ady + (adx >> 1);
}

// Palette index for the pixel at normalized (px, py): layered plasma waves,
// a tilt-following breathing glow, and expanding rings from recent touches.
static inline int field_at(const Frame *f, int px, int py) {
    int plasma = s_sin[(px * 2 + f->t1) & 255]
               + s_sin[(py * 2 - f->t2) & 255]
               + s_sin[((px + py) * 2 + f->t3) & 255];

    const int d = oct_dist(px - f->cx, py - f->cy);
    plasma += s_sin[(d * 3 - f->t1 * 2) & 255];

    int glow = 255 - d;
    if (glow < 0) glow = 0;

    int rip = 0;
    for (int i = 0; i < f->nrip; i++) {
        const int rd = oct_dist(px - f->rip[i].x, py - f->rip[i].y);
        const int window = RIPPLE_REACH - rd;
        if (window <= 0) continue;
        const int ring = (s_sin[(rd * 5 - f->rip[i].phase) & 255] * f->rip[i].env) >> 7;
        rip += ring * window / RIPPLE_REACH;
    }

    int idx = 24 + (glow * (140 + f->breath) >> 9) + plasma / 5 + rip / 2;
    return idx < 0 ? 0 : idx > 255 ? 255 : idx;
}

void ambient_render(const Ambient *a, uint16_t *fb, int w, int h) {
    if (!s_tables_built) {
        build_tables();
    }
    const Frame f = frame_of(a);

    for (int y = 0; y < h; y += 2) {
        const int rows = (y + 1 < h) ? 2 : 1;
        for (int x = 0; x < w; x += 2) {
            const uint16_t c = s_pal[field_at(&f, x * 256 / w, y * 256 / h)];
            const int cols = (x + 1 < w) ? 2 : 1;
            for (int r = 0; r < rows; r++) {
                uint16_t *row = &fb[(y + r) * w + x];
                for (int cc = 0; cc < cols; cc++) {
                    row[cc] = c;
                }
            }
        }
    }
}
