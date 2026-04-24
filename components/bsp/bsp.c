/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Board Support Package implementation for 2.8inch ESP32-S3 Display
 * https://www.lcdwiki.com/zh/2.8inch_ESP32-S3_Display
 *
 * Hardware:
 *   LCD : ILI9341V, 240×320, SPI
 *   Touch: FT6336G, I2C (GPIO15/GPIO16 shared bus)
 *   Audio: ES8311 codec over shared I2C + I2S full-duplex data path
 *          IO1=amp enable(active-low), IO4=MCLK, IO5=BCLK,
 *          IO6=DIN(codec ADC -> ESP), IO7=LRCK, IO8=DOUT(ESP -> codec DAC)
 */

#include "bsp.h"

#include <string.h>
#include "esp_err.h"
#include "esp_check.h"
#include "esp_idf_version.h"
#include "esp_log.h"

#include "driver/gpio.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0) && !CONFIG_CODEC_I2C_BACKWARD_COMPATIBLE
#include "driver/i2c_master.h"
#define BSP_USE_IDF_I2C_MASTER
#else
#include "driver/i2c.h"
#endif
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/i2s_std.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"

#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"

static const char *TAG = "bsp";

/* ===== LEDC backlight config ===== */
#define BSP_BL_LEDC_TIMER    LEDC_TIMER_0
#define BSP_BL_LEDC_MODE     LEDC_LOW_SPEED_MODE
#define BSP_BL_LEDC_CHANNEL  LEDC_CHANNEL_0
#define BSP_BL_LEDC_FREQ_HZ  5000
#define BSP_BL_LEDC_DUTY_RES LEDC_TIMER_10_BIT

/* ===== Audio DMA config ===== */
#define BSP_AUDIO_DMA_DESC_NUM  6
#define BSP_AUDIO_DMA_FRAME_NUM 240

/* ===== Static state ===== */
static bsp_lcd_handles_t s_lcd_handles;
static bsp_lcd_config_t  s_lcd_config;

static i2s_chan_handle_t s_i2s_tx = NULL;
static i2s_chan_handle_t s_i2s_rx = NULL;

#ifdef BSP_USE_IDF_I2C_MASTER
static i2c_master_bus_handle_t s_i2c_bus = NULL;
#endif
static bool s_i2c_initialized = false;

static const audio_codec_data_if_t *s_i2s_if = NULL;
static const audio_codec_ctrl_if_t *s_codec_ctrl_if = NULL;
static const audio_codec_gpio_if_t *s_codec_gpio_if = NULL;
static const audio_codec_if_t *s_codec_if = NULL;

static esp_codec_dev_handle_t s_codec_dev = NULL;

static bool s_initialized = false;

static void bsp_i2s_disable_if_enabled(i2s_chan_handle_t chan)
{
    if (chan == NULL) {
        return;
    }

    i2s_chan_info_t info = {0};
    if (i2s_channel_get_info(chan, &info) == ESP_OK && info.is_enabled) {
        i2s_channel_disable(chan);
    }
}

static esp_err_t bsp_i2c_init(void)
{
    if (s_i2c_initialized) {
        return ESP_OK;
    }

#ifdef BSP_USE_IDF_I2C_MASTER
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = BSP_I2C_PORT,
        .scl_io_num = BSP_TOUCH_SCL_GPIO,
        .sda_io_num = BSP_TOUCH_SDA_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t ret = i2c_new_master_bus(&i2c_bus_config, &s_i2c_bus);
    if (ret != ESP_OK) {
        return ret;
    }
#else
    i2c_config_t i2c_cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BSP_TOUCH_SDA_GPIO,
        .scl_io_num = BSP_TOUCH_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    esp_err_t ret = i2c_param_config(BSP_I2C_PORT, &i2c_cfg);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = i2c_driver_install(BSP_I2C_PORT, i2c_cfg.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        return ret;
    }
#endif

    s_i2c_initialized = true;
    return ESP_OK;
}

static void bsp_i2c_deinit(void)
{
    if (!s_i2c_initialized) {
        return;
    }

#ifdef BSP_USE_IDF_I2C_MASTER
    if (s_i2c_bus) {
        i2c_del_master_bus(s_i2c_bus);
        s_i2c_bus = NULL;
    }
#else
    i2c_driver_delete(BSP_I2C_PORT);
#endif
    s_i2c_initialized = false;
}

static void bsp_audio_cleanup(void)
{
    if (s_codec_dev) {
        esp_codec_dev_delete(s_codec_dev);
        s_codec_dev = NULL;
    }
    if (s_codec_if) {
        audio_codec_delete_codec_if(s_codec_if);
        s_codec_if = NULL;
    }
    if (s_codec_ctrl_if) {
        audio_codec_delete_ctrl_if(s_codec_ctrl_if);
        s_codec_ctrl_if = NULL;
    }
    if (s_codec_gpio_if) {
        audio_codec_delete_gpio_if(s_codec_gpio_if);
        s_codec_gpio_if = NULL;
    }
    if (s_i2s_if) {
        audio_codec_delete_data_if(s_i2s_if);
        s_i2s_if = NULL;
    }
    if (s_i2s_tx) {
        bsp_i2s_disable_if_enabled(s_i2s_tx);
        i2s_del_channel(s_i2s_tx);
        s_i2s_tx = NULL;
    }
    if (s_i2s_rx) {
        bsp_i2s_disable_if_enabled(s_i2s_rx);
        i2s_del_channel(s_i2s_rx);
        s_i2s_rx = NULL;
    }
    bsp_i2c_deinit();
    gpio_set_level(BSP_AUDIO_EN_GPIO, 1);
}

/* ===== Backlight ===== */

static esp_err_t bsp_lcd_backlight_init(void)
{
    ledc_timer_config_t timer_cfg = {
        .speed_mode      = BSP_BL_LEDC_MODE,
        .duty_resolution = BSP_BL_LEDC_DUTY_RES,
        .timer_num       = BSP_BL_LEDC_TIMER,
        .freq_hz         = BSP_BL_LEDC_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_cfg), TAG, "LEDC timer config failed");

    ledc_channel_config_t ch_cfg = {
        .gpio_num   = BSP_LCD_BL_GPIO,
        .speed_mode = BSP_BL_LEDC_MODE,
        .channel    = BSP_BL_LEDC_CHANNEL,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = BSP_BL_LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&ch_cfg), TAG, "LEDC channel config failed");
    return ESP_OK;
}

esp_err_t bsp_set_lcd_backlight(int percent)
{
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;

    uint32_t duty = ((1u << BSP_BL_LEDC_DUTY_RES) - 1) * (uint32_t)percent / 100;
    ledc_set_duty(BSP_BL_LEDC_MODE, BSP_BL_LEDC_CHANNEL, duty);
    ledc_update_duty(BSP_BL_LEDC_MODE, BSP_BL_LEDC_CHANNEL);
    return ESP_OK;
}

/* ===== LCD ===== */

static esp_err_t bsp_lcd_init(void)
{
    /* SPI bus */
    spi_bus_config_t buscfg = {
        .mosi_io_num     = BSP_LCD_MOSI_GPIO,
        .miso_io_num     = BSP_LCD_MISO_GPIO,
        .sclk_io_num     = BSP_LCD_CLK_GPIO,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = BSP_LCD_WIDTH * BSP_LCD_HEIGHT * 2 + 8,
    };
    ESP_RETURN_ON_ERROR(
        spi_bus_initialize(BSP_LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO),
        TAG, "SPI bus init failed");

    /* LCD IO (SPI) */
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num       = BSP_LCD_DC_GPIO,
        .cs_gpio_num       = BSP_LCD_CS_GPIO,
        .pclk_hz           = 40 * 1000 * 1000,
        .lcd_cmd_bits      = 8,
        .lcd_param_bits    = 8,
        .spi_mode          = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx          = NULL,
        .flags             = { .dc_low_on_data = 0, .octal_mode = 0, .sio_mode = 0,
                                .lsb_first = 0, .cs_high_active = 0 },
    };
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BSP_LCD_SPI_HOST,
                                 &io_cfg, &s_lcd_handles.io_handle),
        TAG, "LCD IO init failed");

    /* ILI9341 panel */
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num  = BSP_LCD_RST_GPIO,   /* -1 = not connected */
        .rgb_ele_order   = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel  = 16,
        .vendor_config   = NULL,
    };
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_ili9341(s_lcd_handles.io_handle, &panel_cfg,
                                  &s_lcd_handles.panel_handle),
        TAG, "ILI9341 panel init failed");

    /* Reset, initialize, and turn on the panel */
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_lcd_handles.panel_handle),
                        TAG, "LCD reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_lcd_handles.panel_handle),
                        TAG, "LCD init failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_swap_xy(s_lcd_handles.panel_handle, false),
                        TAG, "LCD swap_xy failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(s_lcd_handles.panel_handle, true, false),
                        TAG, "LCD mirror failed");
    /* ILI9341 typically needs color inversion to get correct colors */
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(s_lcd_handles.panel_handle, true),
                        TAG, "LCD invert color failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_lcd_handles.panel_handle, true),
                        TAG, "LCD display on failed");

    /* Fill config struct */
    s_lcd_config.lcd_width  = BSP_LCD_WIDTH;
    s_lcd_config.lcd_height = BSP_LCD_HEIGHT;
    s_lcd_config.mirror_x   = true;
    s_lcd_config.mirror_y   = false;
    s_lcd_config.swap_xy    = false;

    ESP_LOGI(TAG, "LCD ILI9341 initialized (%ux%u)", BSP_LCD_WIDTH, BSP_LCD_HEIGHT);
    return ESP_OK;
}

/* ===== Audio ===== */

static esp_err_t bsp_audio_init(bsp_audio_info_t *info)
{
    esp_err_t ret = ESP_OK;

    /* Enable audio analog path early so the codec and microphone stay powered */
    gpio_config_t en_cfg = {
        .pin_bit_mask = (1ULL << BSP_AUDIO_EN_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&en_cfg), TAG, "Audio enable GPIO config failed");
    gpio_set_level(BSP_AUDIO_EN_GPIO, 0);   /* Active low: enable audio */

    ret = bsp_i2c_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Audio I2C init failed: %s", esp_err_to_name(ret));
        goto err;
    }

    /* Create full-duplex I2S channels */
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = BSP_AUDIO_DMA_DESC_NUM;
    chan_cfg.dma_frame_num = BSP_AUDIO_DMA_FRAME_NUM;
    chan_cfg.auto_clear = true;
    ret = i2s_new_channel(&chan_cfg, &s_i2s_tx, &s_i2s_rx);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S channel create failed: %s", esp_err_to_name(ret));
        goto err;
    }

    /* Standard (Philips) I2S mode configuration */
    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(BSP_AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
                        I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = BSP_AUDIO_MCLK_GPIO,
            .bclk = BSP_AUDIO_BCLK_GPIO,
            .ws   = BSP_AUDIO_LRCK_GPIO,
            .dout = BSP_AUDIO_DOUT_GPIO,
            .din  = BSP_AUDIO_DIN_GPIO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;

    /* Initialize TX (speaker) channel — not yet enabled; esp_codec_dev opens it */
    ret = i2s_channel_init_std_mode(s_i2s_tx, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S TX init failed: %s", esp_err_to_name(ret));
        goto err;
    }

    /* Initialize RX (microphone) channel */
    ret = i2s_channel_init_std_mode(s_i2s_rx, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S RX init failed: %s", esp_err_to_name(ret));
        goto err;
    }

    ret = i2s_channel_enable(s_i2s_tx);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S TX enable failed: %s", esp_err_to_name(ret));
        goto err;
    }

    ret = i2s_channel_enable(s_i2s_rx);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S RX enable failed: %s", esp_err_to_name(ret));
        goto err;
    }

    audio_codec_i2s_cfg_t i2s_cfg = {
        .tx_handle = s_i2s_tx,
        .rx_handle = s_i2s_rx,
    };
    s_i2s_if = audio_codec_new_i2s_data(&i2s_cfg);
    if (s_i2s_if == NULL) {
        ret = ESP_FAIL;
        ESP_LOGE(TAG, "I2S duplex data IF create failed");
        goto err;
    }

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = BSP_I2C_PORT,
        .addr = ES8311_CODEC_DEFAULT_ADDR,
#ifdef BSP_USE_IDF_I2C_MASTER
        .bus_handle = s_i2c_bus,
#endif
    };
    s_codec_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    if (s_codec_ctrl_if == NULL) {
        ret = ESP_FAIL;
        ESP_LOGE(TAG, "ES8311 I2C control IF create failed");
        goto err;
    }

    s_codec_gpio_if = audio_codec_new_gpio();
    if (s_codec_gpio_if == NULL) {
        ret = ESP_FAIL;
        ESP_LOGE(TAG, "ES8311 GPIO IF create failed");
        goto err;
    }

    es8311_codec_cfg_t codec_cfg = {
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH,
        .ctrl_if = s_codec_ctrl_if,
        .gpio_if = s_codec_gpio_if,
        .pa_pin = -1,
        .use_mclk = true,
        .hw_gain = {
            .pa_voltage = 5.0,
            .codec_dac_voltage = 3.3,
        },
    };
    s_codec_if = es8311_codec_new(&codec_cfg);
    if (s_codec_if == NULL) {
        ret = ESP_FAIL;
        ESP_LOGE(TAG, "ES8311 codec IF create failed");
        goto err;
    }

    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type  = ESP_CODEC_DEV_TYPE_IN_OUT,
        .codec_if  = s_codec_if,
        .data_if   = s_i2s_if,
    };
    s_codec_dev = esp_codec_dev_new(&dev_cfg);
    if (s_codec_dev == NULL) {
        ret = ESP_FAIL;
        ESP_LOGE(TAG, "Codec device create failed");
        goto err;
    }

    if (info) {
        info->play_dev   = s_codec_dev;
        info->rec_dev    = s_codec_dev;
        strncpy(info->mic_layout, "M", sizeof(info->mic_layout) - 1);
        info->mic_layout[sizeof(info->mic_layout) - 1] = '\0';
        info->sample_rate = BSP_AUDIO_SAMPLE_RATE;
        info->sample_bits = 16;
        info->channels    = 1;
    }

    ESP_LOGI(TAG, "Audio ES8311 initialized (rate=%d, bits=16, logical-mono, stereo-slot)", BSP_AUDIO_SAMPLE_RATE);
    return ESP_OK;

err:
    bsp_audio_cleanup();
    return ret;
}

/* ===== Public API ===== */

esp_err_t bsp_init(bsp_audio_info_t *audio_info)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "BSP already initialized");
        if (audio_info) {
            audio_info->play_dev    = s_codec_dev;
            audio_info->rec_dev     = s_codec_dev;
            strncpy(audio_info->mic_layout, "M", sizeof(audio_info->mic_layout) - 1);
            audio_info->mic_layout[sizeof(audio_info->mic_layout) - 1] = '\0';
            audio_info->sample_rate = BSP_AUDIO_SAMPLE_RATE;
            audio_info->sample_bits = 16;
            audio_info->channels    = 1;
        }
        return ESP_OK;
    }

    esp_err_t ret = ESP_OK;

    ret = bsp_lcd_backlight_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Backlight init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = bsp_lcd_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = bsp_audio_init(audio_info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Audio init failed: %s", esp_err_to_name(ret));
        bsp_deinit();
        return ret;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "BSP initialized for 2.8inch ESP32-S3 Display");
    return ESP_OK;
}

esp_err_t bsp_deinit(void)
{
    bsp_audio_cleanup();
    if (s_lcd_handles.panel_handle) {
        esp_lcd_panel_del(s_lcd_handles.panel_handle);
        s_lcd_handles.panel_handle = NULL;
    }
    if (s_lcd_handles.io_handle) {
        esp_lcd_panel_io_del(s_lcd_handles.io_handle);
        s_lcd_handles.io_handle = NULL;
    }
    spi_bus_free(BSP_LCD_SPI_HOST);

    s_initialized = false;
    return ESP_OK;
}

bool bsp_is_initialized(void)
{
    return s_initialized;
}

esp_err_t bsp_get_lcd_handles_ptr(bsp_lcd_handles_t **handles)
{
    ESP_RETURN_ON_FALSE(handles != NULL, ESP_ERR_INVALID_ARG, TAG, "handles is NULL");
    if (!s_initialized || s_lcd_handles.panel_handle == NULL) {
        *handles = NULL;
        return ESP_ERR_INVALID_STATE;
    }
    *handles = &s_lcd_handles;
    return ESP_OK;
}

esp_err_t bsp_get_lcd_config_ptr(bsp_lcd_config_t **config)
{
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config is NULL");
    *config = &s_lcd_config;
    return ESP_OK;
}
