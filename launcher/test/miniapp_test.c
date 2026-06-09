// Host unit tests for the Mini-app registry — pure logic, no hardware.
//
// A recording fake stands in for a real Mini-app: each lifecycle hook appends a
// marker to a shared log, so tests assert lifecycle ordering and that only the
// active Mini-app runs — behavior, not internal structure.

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "miniapp.h"

static int g_checks = 0;
#define CHECK(cond) do { assert(cond); g_checks++; } while (0)

// Shared event log + a fake Mini-app whose hooks record into it.
typedef struct {
    const char *name;
    char *log;
} Fake;

static void rec(Fake *f, const char *ev) {
    strcat(f->log, ev);
    strcat(f->log, ":");
    strcat(f->log, f->name);
    strcat(f->log, ";");
}
static void f_enter(void *c)  { rec((Fake *)c, "enter"); }
static void f_exit(void *c)   { rec((Fake *)c, "exit"); }
static void f_tick(void *c)   { rec((Fake *)c, "tick"); }
static void f_render(void *c) { rec((Fake *)c, "render"); }

static MiniApp make_fake(Fake *f) {
    MiniApp a = {
        .name = f->name,
        .on_enter = f_enter, .on_exit = f_exit,
        .on_tick = f_tick, .render = f_render,
        .ctx = f,
    };
    return a;
}

// Activating a registered Mini-app enters it and makes it active.
static void test_activate_enters(void) {
    char log[256] = {0};
    Fake fa = {.name = "A", .log = log};
    MiniApp a = make_fake(&fa);

    MiniAppRegistry reg;
    miniapp_registry_init(&reg);
    int ia = miniapp_register(&reg, &a);

    miniapp_set_active(&reg, ia);

    CHECK(miniapp_active(&reg) == &a);
    CHECK(strcmp(log, "enter:A;") == 0);
}

// Switching active exits the old then enters the new, in that order.
static void test_switch_exits_then_enters(void) {
    char log[256] = {0};
    Fake fa = {.name = "A", .log = log};
    Fake fb = {.name = "B", .log = log};
    MiniApp a = make_fake(&fa), b = make_fake(&fb);

    MiniAppRegistry reg;
    miniapp_registry_init(&reg);
    int ia = miniapp_register(&reg, &a);
    int ib = miniapp_register(&reg, &b);

    miniapp_set_active(&reg, ia);  // enter:A
    miniapp_set_active(&reg, ib);  // exit:A; enter:B

    CHECK(miniapp_active(&reg) == &b);
    CHECK(strcmp(log, "enter:A;exit:A;enter:B;") == 0);
}

// Only the active Mini-app ticks and renders, in that order.
static void test_only_active_ticks_and_renders(void) {
    char log[256] = {0};
    Fake fa = {.name = "A", .log = log};
    Fake fb = {.name = "B", .log = log};
    MiniApp a = make_fake(&fa), b = make_fake(&fb);

    MiniAppRegistry reg;
    miniapp_registry_init(&reg);
    miniapp_register(&reg, &a);
    int ib = miniapp_register(&reg, &b);
    miniapp_set_active(&reg, ib);

    log[0] = '\0';            // ignore enter noise; focus on the tick
    miniapp_tick(&reg);

    // B (active) ran tick then render; A never ran.
    CHECK(strcmp(log, "tick:B;render:B;") == 0);
}

// Re-activating the already-active Mini-app is a no-op (no exit/enter churn).
static void test_reactivate_is_noop(void) {
    char log[256] = {0};
    Fake fa = {.name = "A", .log = log};
    MiniApp a = make_fake(&fa);

    MiniAppRegistry reg;
    miniapp_registry_init(&reg);
    int ia = miniapp_register(&reg, &a);
    miniapp_set_active(&reg, ia);
    log[0] = '\0';
    miniapp_set_active(&reg, ia);  // same index

    CHECK(log[0] == '\0');
}

// An invalid index leaves the active Mini-app unchanged.
static void test_invalid_index_ignored(void) {
    char log[256] = {0};
    Fake fa = {.name = "A", .log = log};
    MiniApp a = make_fake(&fa);

    MiniAppRegistry reg;
    miniapp_registry_init(&reg);
    int ia = miniapp_register(&reg, &a);
    miniapp_set_active(&reg, ia);

    miniapp_set_active(&reg, 99);
    miniapp_set_active(&reg, -1);

    CHECK(miniapp_active(&reg) == &a);
}

int main(void) {
    test_activate_enters();
    test_switch_exits_then_enters();
    test_only_active_ticks_and_renders();
    test_reactivate_is_noop();
    test_invalid_index_ignored();
    printf("miniapp: all %d checks passed\n", g_checks);
    return 0;
}
