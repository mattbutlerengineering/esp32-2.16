// Host unit tests for the Settings config logic — clamping and Bridge-URL
// validation. Pure logic; NVS persistence and WiFi provisioning are hardware and
// live elsewhere.

#include <assert.h>
#include <stdio.h>

#include "config.h"

static int g_checks = 0;
#define CHECK(cond) do { assert(cond); g_checks++; } while (0)

// Brightness/volume are percentages clamped to 0..100.
static void test_clamp_percent(void) {
    CHECK(config_clamp_percent(50) == 50);
    CHECK(config_clamp_percent(-5) == 0);
    CHECK(config_clamp_percent(0) == 0);
    CHECK(config_clamp_percent(100) == 100);
    CHECK(config_clamp_percent(150) == 100);
}

// A usable Bridge URL is http(s):// followed by a host.
static void test_valid_bridge_url(void) {
    CHECK(config_valid_bridge_url("http://192.168.1.5:8080/talk"));
    CHECK(config_valid_bridge_url("https://orb.local"));
}

// Reject empties, scheme-only, wrong scheme, and null.
static void test_invalid_bridge_url(void) {
    CHECK(!config_valid_bridge_url(""));
    CHECK(!config_valid_bridge_url("http://"));
    CHECK(!config_valid_bridge_url("https://"));
    CHECK(!config_valid_bridge_url("ftp://host"));
    CHECK(!config_valid_bridge_url("192.168.1.5:8080"));
    CHECK(!config_valid_bridge_url(NULL));
}

int main(void) {
    test_clamp_percent();
    test_valid_bridge_url();
    test_invalid_bridge_url();
    printf("config: all %d checks passed\n", g_checks);
    return 0;
}
