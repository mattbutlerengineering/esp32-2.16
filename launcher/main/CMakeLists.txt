idf_component_register(
    SRCS
        "launcher_main.c"
        "bsp/pmic_axp2101.c"
        "bsp/display.c"
        "bsp/touch_cst9217.c"
        "bsp/audio.c"
        "bsp/wifi.c"
        "bsp/imu_qmi8658.c"
        "net/bridge_client.c"
        "audio/wav.c"
        "settings/config.c"
        "voice/voice_session.c"
        "orb/orb_fsm.c"
        "app/miniapp.c"
        "app/ambient.c"
        "app/ambient_app.c"
    INCLUDE_DIRS "." "bsp" "orb" "app" "audio" "settings" "voice" "net"
    REQUIRES nvs_flash esp_wifi esp_netif esp_event esp_http_client
)
