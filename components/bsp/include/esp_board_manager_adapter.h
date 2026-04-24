/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Compatibility stub: replaces esp_board_manager_adapter.h from esp-iot-solution.
 * Maps esp_board_manager_adapter API to the custom BSP for the 2.8inch ESP32-S3 Display.
 */

#pragma once

#include "esp_err.h"
#include "esp_codec_dev.h"
#include "bsp.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Board manager adapter initialization configuration
 */
typedef struct {
    bool enable_lcd;            /*!< Enable LCD initialization */
    bool enable_lcd_backlight;  /*!< Enable LCD backlight */
    bool enable_lvgl;           /*!< Enable LVGL (not used, managed by display module) */
} esp_board_manager_adapter_config_t;

/** Default adapter config */
#define ESP_BOARD_MANAGER_ADAPTER_CONFIG_DEFAULT() { \
    .enable_lcd = true,            \
    .enable_lcd_backlight = true,  \
    .enable_lvgl = false           \
}

/** Board manager adapter info — mapped directly to bsp_audio_info_t */
typedef bsp_audio_info_t esp_board_manager_adapter_info_t;

/**
 * @brief Initialize board peripherals (mapped to bsp_init)
 */
static inline esp_err_t esp_board_manager_adapter_init(
    esp_board_manager_adapter_config_t *config,
    esp_board_manager_adapter_info_t *info)
{
    (void)config;  /* BSP initializes all peripherals unconditionally */
    return bsp_init(info);
}

/**
 * @brief Deinitialize board peripherals (mapped to bsp_deinit)
 */
static inline esp_err_t esp_board_manager_adapter_deinit(void)
{
    return bsp_deinit();
}

/**
 * @brief Set LCD backlight brightness (mapped to bsp_set_lcd_backlight)
 */
static inline esp_err_t esp_board_manager_adapter_set_lcd_backlight(int percent)
{
    return bsp_set_lcd_backlight(percent);
}

#ifdef __cplusplus
}
#endif
