// WiFi station: connect using credentials stored in NVS. On first boot (empty
// NVS) the Kconfig defaults are seeded into NVS, so credentials are owned by the
// device, not the source tree. Connecting is asynchronous and auto-retries.

#include "bsp/wifi.h"

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_check.h"
#include "esp_log.h"

#include <string.h>

static const char *TAG = "wifi";

#define WIFI_NVS_NS   "orb"
#define WIFI_MAX_RETRY 8

static bool s_connected = false;
static int  s_retry = 0;

static void on_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_connected = false;
        if (s_retry < WIFI_MAX_RETRY) {
            s_retry++;
            ESP_LOGW(TAG, "disconnected, retry %d/%d", s_retry, WIFI_MAX_RETRY);
            esp_wifi_connect();
        } else {
            ESP_LOGE(TAG, "giving up after %d retries — staying offline", WIFI_MAX_RETRY);
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "got IP " IPSTR, IP2STR(&e->ip_info.ip));
        s_retry = 0;
        s_connected = true;
    }
}

// Load SSID/password from NVS; if absent, seed from Kconfig and persist. Returns
// false when no SSID is configured anywhere.
static bool load_creds(char *ssid, size_t ssid_sz, char *pass, size_t pass_sz)
{
    nvs_handle_t h;
    if (nvs_open(WIFI_NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        size_t sl = ssid_sz, pl = pass_sz;
        bool ok = (nvs_get_str(h, "ssid", ssid, &sl) == ESP_OK) && ssid[0] != '\0';
        if (ok && nvs_get_str(h, "pass", pass, &pl) != ESP_OK) {
            pass[0] = '\0';
        }
        nvs_close(h);
        if (ok) {
            return true;
        }
    }

    // First boot / not provisioned: seed from Kconfig.
    strlcpy(ssid, CONFIG_ORB_WIFI_SSID, ssid_sz);
    strlcpy(pass, CONFIG_ORB_WIFI_PASSWORD, pass_sz);
    if (ssid[0] == '\0') {
        return false;
    }
    if (nvs_open(WIFI_NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, "ssid", ssid);
        nvs_set_str(h, "pass", pass);
        nvs_commit(h);
        nvs_close(h);
        ESP_LOGI(TAG, "seeded WiFi credentials from Kconfig into NVS");
    }
    return true;
}

esp_err_t wifi_start(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_RETURN_ON_ERROR(err, TAG, "nvs init");

    char ssid[33] = {0}, pass[65] = {0};
    if (!load_creds(ssid, sizeof(ssid), pass, sizeof(pass))) {
        ESP_LOGW(TAG, "no WiFi SSID configured (set via `idf.py menuconfig`) — offline");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "netif init");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "event loop");
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "wifi init");
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        on_event, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        on_event, NULL, NULL));

    wifi_config_t wc = {0};
    strlcpy((char *)wc.sta.ssid, ssid, sizeof(wc.sta.ssid));
    strlcpy((char *)wc.sta.password, pass, sizeof(wc.sta.password));
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "set mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wc), TAG, "set config");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "wifi start");

    ESP_LOGI(TAG, "connecting to \"%s\" (async)", ssid);
    return ESP_OK;
}

bool wifi_is_connected(void)
{
    return s_connected;
}
