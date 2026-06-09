// Mini-app registry + lifecycle for the Launcher.
//
// A Mini-app is a self-contained mode (Orb, Motion, Settings, ...) hosted inside
// the Launcher. The registry holds them, tracks which one is the foreground
// (active) Mini-app, and dispatches a uniform lifecycle. Only the active
// Mini-app ticks and renders. Pure logic, host-unit-testable.
#pragma once

#include <stddef.h>

#define MINIAPP_MAX 8

typedef struct {
    const char *name;
    void (*on_enter)(void *ctx);  // becoming the active Mini-app
    void (*on_exit)(void *ctx);   // leaving the foreground
    void (*on_tick)(void *ctx);   // per-frame update while active
    void (*render)(void *ctx);    // draw while active
    void *ctx;
} MiniApp;

typedef struct {
    const MiniApp *apps[MINIAPP_MAX];
    int count;
    int active;  // index of the active Mini-app, or -1 if none
} MiniAppRegistry;

void miniapp_registry_init(MiniAppRegistry *reg);

// Register a Mini-app. Returns its index, or -1 if the registry is full.
int miniapp_register(MiniAppRegistry *reg, const MiniApp *app);

// Make the Mini-app at `index` the active one: exits the current active (if any),
// then enters the new one. No-op if `index` is invalid or already active.
void miniapp_set_active(MiniAppRegistry *reg, int index);

// The active Mini-app, or NULL if none.
const MiniApp *miniapp_active(const MiniAppRegistry *reg);

// Tick + render the active Mini-app (only the active one runs).
void miniapp_tick(MiniAppRegistry *reg);
