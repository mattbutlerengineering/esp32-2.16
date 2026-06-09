#include "config.h"

#include <string.h>

int config_clamp_percent(int value) {
    if (value < 0) return 0;
    if (value > 100) return 100;
    return value;
}

bool config_valid_bridge_url(const char *url) {
    if (url == NULL) return false;

    const char *host = NULL;
    if (strncmp(url, "http://", 7) == 0) {
        host = url + 7;
    } else if (strncmp(url, "https://", 8) == 0) {
        host = url + 8;
    } else {
        return false;
    }

    // Require a non-empty host after the scheme.
    return host[0] != '\0';
}
