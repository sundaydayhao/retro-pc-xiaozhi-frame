/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Board Support Package for 2.8inch ESP32-S3 Display
 * https://www.lcdwiki.com/zh/2.8inch_ESP32-S3_Display
 */

#pragma once

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_codec_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===== Board GPIO Definitions ===== */

/* LCD (ILI9341V, SPI) */
#define BSP_LCD_CS_GPIO     GPIO_NUM_10
#define BSP_LCD_DC_GPIO     GPIO_NUM_46
#define BSP_LCD_CLK_GPIO    GPIO_NUM_12
#define BSP_LCD_MOSI_GPIO   GPIO_NUM_11
#define BSP_LCD_MISO_GPIO   GPIO_NUM_13
#define BSP_LCD_RST_GPIO    GPIO_NUM_NC   /*!< Tied to MCU reset (EN pin) */
#define BSP_LCD_BL_GPIO     GPIO_NUM_45
#define BSP_LCD_WIDTH       240
#define BSP_LCD_HEIGHT      320
#define BSP_LCD_SPI_HOST    SPI2_HOST

/* Touch (FT6336G, I2C) */
#define BSP_TOUCH_SCL_GPIO  GPIO_NUM_15
#define BSP_TOUCH_SDA_GPIO  GPIO_NUM_16
#define BSP_TOUCH_RST_GPIO  GPIO_NUM_18
#define BSP_TOUCH_INT_GPIO  GPIO_NUM_17
#define BSP_TOUCH_I2C_ADDR  0x38
#define BSP_I2C_PORT        I2C_NUM_0

/* Audio (ES8311 codec: I2C control + I2S data) */
#define BSP_AUDIO_EN_GPIO   GPIO_NUM_1   /*!< Audio amplifier enable, active low */
#define BSP_AUDIO_MCLK_GPIO GPIO_NUM_4
#define BSP_AUDIO_BCLK_GPIO GPIO_NUM_5
#define BSP_AUDIO_DOUT_GPIO GPIO_NUM_8   /*!< I2S data out from ESP to codec DAC */
#define BSP_AUDIO_LRCK_GPIO GPIO_NUM_7
#define BSP_AUDIO_DIN_GPIO  GPIO_NUM_6   /*!< I2S data in from codec ADC to ESP */
#define BSP_AUDIO_SAMPLE_RATE 16000

/* SD Card (SDIO 4-bit) */
#define BSP_SD_CLK_GPIO     GPIO_NUM_38
#define BSP_SD_CMD_GPIO     GPIO_NUM_40
#define BSP_SD_D0_GPIO      GPIO_NUM_39
#define BSP_SD_D1_GPIO      GPIO_NUM_41
#define BSP_SD_D2_GPIO      GPIO_NUM_48
#define BSP_SD_D3_GPIO      GPIO_NUM_47

/* RGB LED */
#define BSP_LED_GPIO        GPIO_NUM_42

/* Battery ADC */
#define BSP_BAT_ADC_GPIO    GPIO_NUM_9

/* ===== BSP Data Types ===== */

/**
 * @brief LCD panel handles
 */
typedef struct {
    esp_lcd_panel_io_handle_t io_handle;     /*!< LCD panel IO handle */
    esp_lcd_panel_handle_t    panel_handle;  /*!< LCD panel handle */
} bsp_lcd_handles_t;

/**
 * @brief LCD panel configuration
 */
typedef struct {
    uint32_t lcd_width;   /*!< Panel width in pixels */
    uint32_t lcd_height;  /*!< Panel height in pixels */
    bool     mirror_x;    /*!< Mirror X axis */
    bool     mirror_y;    /*!< Mirror Y axis */
    bool     swap_xy;     /*!< Swap X/Y axes */
} bsp_lcd_config_t;

/**
 * @brief Audio devices info returned by bsp_init()
 */
typedef struct {
    esp_codec_dev_handle_t play_dev;     /*!< Playback codec device handle */
    esp_codec_dev_handle_t rec_dev;      /*!< Recording codec device handle */
    char                   mic_layout[32]; /*!< Microphone layout string, e.g. "M", "MR" */
    int                    sample_rate;  /*!< Audio sample rate (Hz) */
    int                    sample_bits;  /*!< Audio sample bits */
    int                    channels;     /*!< Audio channels */
} bsp_audio_info_t;

/* ===== BSP API ===== */

/**
 * @brief Initialize all board peripherals
 *
 * Initializes: SPI bus, LCD panel, LEDC backlight, I2S audio
 *
 * @param[out] audio_info Optional pointer to receive audio configuration.
 *                        Pass NULL if audio info is not needed.
 * @return ESP_OK on success
 */
esp_err_t bsp_init(bsp_audio_info_t *audio_info);

/**
 * @brief Deinitialize board peripherals
 *
 * @return ESP_OK on success
 */
esp_err_t bsp_deinit(void);

/**
 * @brief Query whether the BSP has been initialized
 *
 * @return true if bsp_init() has completed successfully
 */
bool bsp_is_initialized(void);

/**
 * @brief Set LCD backlight brightness via LEDC PWM
 *
 * @param[in] percent Brightness percentage (0-100)
 * @return ESP_OK on success
 */
esp_err_t bsp_set_lcd_backlight(int percent);

/**
 * @brief Get pointer to LCD panel handles
 *
 * Must be called after bsp_init().
 *
 * @param[out] handles Pointer to receive pointer to LCD handles struct
 * @return ESP_OK on success, ESP_ERR_INVALID_STATE if not initialized
 */
esp_err_t bsp_get_lcd_handles_ptr(bsp_lcd_handles_t **handles);

/**
 * @brief Get pointer to LCD panel configuration
 *
 * Must be called after bsp_init().
 *
 * @param[out] config Pointer to receive pointer to LCD config struct
 * @return ESP_OK on success
 */
esp_err_t bsp_get_lcd_config_ptr(bsp_lcd_config_t **config);

#ifdef __cplusplus
}
#endif
