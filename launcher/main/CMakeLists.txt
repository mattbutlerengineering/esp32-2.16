idf_component_register(
    SRCS
        "launcher_main.c"
        "bsp/pmic_axp2101.c"
        "bsp/display.c"
        "bsp/touch_cst9217.c"
        "bsp/audio.c"
        "bsp/wifi.c"
        "orb/orb_fsm.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi esp_netif esp_event
)
