#include "miniapp.h"

void miniapp_registry_init(MiniAppRegistry *reg) {
    reg->count = 0;
    reg->active = -1;
}

int miniapp_register(MiniAppRegistry *reg, const MiniApp *app) {
    if (reg->count >= MINIAPP_MAX) return -1;
    int index = reg->count;
    reg->apps[index] = app;
    reg->count++;
    return index;
}

void miniapp_set_active(MiniAppRegistry *reg, int index) {
    if (index < 0 || index >= reg->count) return;
    if (index == reg->active) return;

    if (reg->active >= 0) {
        const MiniApp *cur = reg->apps[reg->active];
        if (cur->on_exit) cur->on_exit(cur->ctx);
    }
    reg->active = index;
    const MiniApp *next = reg->apps[index];
    if (next->on_enter) next->on_enter(next->ctx);
}

const MiniApp *miniapp_active(const MiniAppRegistry *reg) {
    if (reg->active < 0) return NULL;
    return reg->apps[reg->active];
}

void miniapp_tick(MiniAppRegistry *reg) {
    const MiniApp *app = miniapp_active(reg);
    if (!app) return;
    if (app->on_tick) app->on_tick(app->ctx);
    if (app->render) app->render(app->ctx);
}
