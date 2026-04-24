set(SOURCES
    "app_main.c"
    "esp_xiaozhi_chat_app.c"
    "esp_xiaozhi_chat_display.c"
)
set(INCLUDE_DIRS ".")

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES bsp lvgl__lvgl espressif__esp_lvgl_port
    PRIV_REQUIRES nvs_flash esp_event esp_netif esp_timer esp_lcd
    WHOLE_ARCHIVE
)
