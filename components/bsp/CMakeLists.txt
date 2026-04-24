idf_component_register(
    SRCS
        "bsp.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        esp_lcd
        driver
        esp_timer
        esp_driver_i2s
)

