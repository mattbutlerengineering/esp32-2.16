#include "motion_app.h"

void motion_app_init(MotionApp *app, ImuReadFn read, void *ctx) {
    app->read = read;
    app->ctx = ctx;
    app->bubble = (Bubble){0.0f, 0.0f};
}

void motion_app_tick(MotionApp *app) {
    float ax = 0.0f, ay = 0.0f;
    if (app->read) app->read(app->ctx, &ax, &ay);
    app->bubble = tilt_bubble(ax, ay);
}

Bubble motion_app_bubble(const MotionApp *app) {
    return app->bubble;
}
