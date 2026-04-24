/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_log.h>
#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_event.h>
#include "protocol_examples_common.h"
#include "esp_board_manager_adapter.h"
#include "esp_xiaozhi_chat_app.h"
#include "esp_xiaozhi_chat_display.h"

static char *TAG = "app_main";

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    esp_err_t wifi_ret = ESP_OK;
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS flash to fix corruption");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_board_manager_adapter_config_t bsp_config = ESP_BOARD_MANAGER_ADAPTER_CONFIG_DEFAULT();
    bsp_config.enable_lcd = true;
    bsp_config.enable_lcd_backlight = true;
    bsp_config.enable_lvgl = false;

    ret = esp_board_manager_adapter_init(&bsp_config, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init board for boot UI: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_xiaozhi_chat_display_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init display for boot UI: %s", esp_err_to_name(ret));
        return;
    }

    esp_xiaozhi_chat_display_set_status("Connecting WiFi...");

    wifi_ret = example_connect();
    if (wifi_ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi connect failed: %s, continue boot without network",
                 esp_err_to_name(wifi_ret));
        esp_xiaozhi_chat_display_set_status("WiFi Failed");
        esp_xiaozhi_chat_display_set_notification("Check WiFi", 3000);
        esp_xiaozhi_chat_display_set_emotion("sad");
        return;
    }

    esp_xiaozhi_chat_display_set_status("Initializing Chat...");

    ret = esp_xiaozhi_chat_app();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Xiaozhi chat app: %s", esp_err_to_name(ret));
        esp_xiaozhi_chat_display_set_status("Chat Init Failed");
        esp_xiaozhi_chat_display_set_notification("Check Network", 3000);
        esp_xiaozhi_chat_display_set_emotion("sad");
        return;
    }
}
