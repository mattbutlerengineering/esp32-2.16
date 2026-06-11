#include "ambient_app.h"

void ambient_app_init(AmbientApp *app, AmbientTouchFn touch, void *touch_ctx,
                      AmbientImuFn imu, void *imu_ctx, int w, int h) {
    app->touch = touch;
    app->touch_ctx = touch_ctx;
    app->imu = imu;
    app->imu_ctx = imu_ctx;
    app->w = w;
    app->h = h;
    app->was_touched = false;
    ambient_init(&app->ambient);
}

void ambient_app_tick(AmbientApp *app, float dt) {
    uint16_t tx = 0, ty = 0;
    const bool touched = app->touch(app->touch_ctx, &tx, &ty);
    if (touched && !app->was_touched) {  // new contact, not a held finger
        ambient_touch(&app->ambient,
                      (float)tx / (float)(app->w - 1),
                      (float)ty / (float)(app->h - 1));
    }
    app->was_touched = touched;

    float ax = 0.0f, ay = 0.0f;
    app->imu(app->imu_ctx, &ax, &ay);
    ambient_tick(&app->ambient, dt, ax, ay);
}

const Ambient *ambient_app_ambient(const AmbientApp *app) {
    return &app->ambient;
}
