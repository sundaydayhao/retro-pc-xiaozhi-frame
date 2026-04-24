/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

esp_err_t esp_xiaozhi_chat_display_init(void);
esp_err_t esp_xiaozhi_chat_display_deinit(void);
bool esp_xiaozhi_chat_display_is_initialized(void);
esp_err_t esp_xiaozhi_chat_display_get_width(int *width);
esp_err_t esp_xiaozhi_chat_display_get_height(int *height);
esp_err_t esp_xiaozhi_chat_display_set_status(const char *status);
esp_err_t esp_xiaozhi_chat_display_set_subtitle(const char *subtitle);
esp_err_t esp_xiaozhi_chat_display_set_volume(int volume);
esp_err_t esp_xiaozhi_chat_display_set_brightness(int brightness);
esp_err_t esp_xiaozhi_chat_display_set_notification(const char *notification, int duration_ms);
esp_err_t esp_xiaozhi_chat_display_set_emotion(const char *emotion);
esp_err_t esp_xiaozhi_chat_display_set_chat_message(const char *role, const char *content);
esp_err_t esp_xiaozhi_chat_display_set_icon(const char *icon);
esp_err_t esp_xiaozhi_chat_display_set_image(const void *image);
esp_err_t esp_xiaozhi_chat_display_set_theme(const char *theme_name);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
