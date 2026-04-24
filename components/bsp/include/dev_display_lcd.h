/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Compatibility stub: replaces dev_display_lcd.h from esp_board_manager
 */

#pragma once

#include "bsp.h"

/* Type aliases for compatibility with esp_board_manager display types */
typedef bsp_lcd_handles_t dev_display_lcd_handles_t;
typedef bsp_lcd_config_t  dev_display_lcd_config_t;
