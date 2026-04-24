/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Compatibility stub: replaces esp_board_manager.h from esp-iot-solution.
 * Provides esp_board_manager_get_device_handle / get_device_config APIs
 * backed by the custom BSP for the 2.8inch ESP32-S3 Display.
 */

#pragma once

#include <string.h>
#include "esp_err.h"
#include "bsp.h"
#include "dev_display_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get a device handle by name
 *
 * Supported names:
 *   "display_lcd" — returns pointer to bsp_lcd_handles_t (as dev_display_lcd_handles_t)
 *
 * @param[in]  name   Device name string
 * @param[out] handle Pointer to receive the device handle
 * @return ESP_OK, ESP_ERR_NOT_FOUND, or ESP_ERR_INVALID_STATE
 */
static inline esp_err_t esp_board_manager_get_device_handle(const char *name, void **handle)
{
    if (strcmp(name, "display_lcd") == 0) {
        return bsp_get_lcd_handles_ptr((bsp_lcd_handles_t **)handle);
    }
    *handle = NULL;
    return ESP_ERR_NOT_FOUND;
}

/**
 * @brief Get a device configuration by name
 *
 * Supported names:
 *   "display_lcd" — returns pointer to bsp_lcd_config_t (as dev_display_lcd_config_t)
 *
 * @param[in]  name   Device name string
 * @param[out] config Pointer to receive the device config
 * @return ESP_OK, ESP_ERR_NOT_FOUND
 */
static inline esp_err_t esp_board_manager_get_device_config(const char *name, void **config)
{
    if (strcmp(name, "display_lcd") == 0) {
        return bsp_get_lcd_config_ptr((bsp_lcd_config_t **)config);
    }
    *config = NULL;
    return ESP_ERR_NOT_FOUND;
}

#ifdef __cplusplus
}
#endif
