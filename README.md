# 小智聊天机器人 — 2.8inch ESP32-S3 Display

基于 [espressif/esp-iot-solution](https://github.com/espressif/esp-iot-solution) `examples/ai/xiaozhi_chat` 移植到 [2.8inch ESP32-S3 Display](https://www.lcdwiki.com/zh/2.8inch_ESP32-S3_Display) 开发板的语音聊天机器人固件。

---

## 功能

- 语音唤醒词：**你好小智**
- 全双工音频：I2S + ES8311 编解码器，支持说话人检测 (VAD) 与 AFE 降噪
- 音频编码：OPUS 24kbps，通过 WebSocket 或 MQTT 与小智后端通信
- 240×320 彩色触摸屏 (ILI9341V)，LVGL 界面：情绪表情、对话文本、状态栏
- 亮暗主题切换、屏幕亮度调节
- MCP 工具集（AI 可调用）：获取设备状态、设置音量/亮度/主题/HSV/RGB

---

## 硬件

| 外设 | GPIO |
|---|---|
| LCD CS | IO10 |
| LCD DC | IO46 |
| LCD CLK | IO12 |
| LCD MOSI | IO11 |
| LCD MISO | IO13 |
| LCD 背光 (LEDC PWM) | IO45 |
| 触摸 SCL | IO15 |
| 触摸 SDA | IO16 |
| 触摸 RST | IO18 |
| 触摸 INT | IO17 |
| 音频使能（低有效）| IO1 |
| I2S MCLK | IO4 |
| I2S BCLK | IO5 |
| I2S DOUT（扬声器）| IO6 |
| I2S LRCK | IO7 |
| I2S DIN（麦克风）| IO8 |
| SD CLK/CMD/D0-D3 | IO38–41, IO47–48 |
| RGB LED | IO42 |

- LCD：ILI9341V，240×320，SPI2\_HOST
- 触摸：FT6336G，I2C
- 音频：ES8311 编解码器，I2C 控制 + I2S 全双工数据

---

## 软件架构

```
main/
  app_main.c                  启动入口：NVS、网络、BSP、显示、小智 App
  esp_xiaozhi_chat_app.c      小智聊天核心：音频管道、MCP 工具注册、事件处理
  esp_xiaozhi_chat_display.c  LVGL 显示层：情绪表情、状态、通知、主题切换

components/bsp/
  bsp.c                       板级驱动：ILI9341 LCD、ES8311 音频、LEDC 背光
  include/bsp.h               BSP 公共 API
  include/esp_board_manager*.h  兼容上游 esp_board_manager_adapter 的 stub 头文件
  include/dev_display_lcd.h   LCD 句柄 stub
```

上游代码依赖 `esp_board_manager_adapter`，本项目用 `components/bsp/` 的 stub 头文件将其替换为直接驱动，**主应用层代码无需修改**。

---

## 依赖

| 组件 | 说明 |
|---|---|
| `espressif/esp_xiaozhi` | 小智 Agent SDK（WebSocket/MQTT 传输、MCP 引擎） |
| `jason-mao/av_processor` | OPUS 编解码、AFE 唤醒词检测、音频管理器 |
| `78/xiaozhi-fonts` | 普惠中文字体 + Font Awesome 表情图标 |
| `protocol_examples_common` | IDF WiFi 连接示例组件 |

---

## 构建与烧录

### 环境要求

- ESP-IDF **>= 5.5.0**
- 目标芯片：ESP32-S3

### 步骤

```bash
# 1. 设置目标芯片
idf.py set-target esp32s3

# 2. 配置 WiFi（Example Connection Configuration → SSID & Password）
idf.py menuconfig

# 3. 编译
idf.py build

# 4. 烧录并查看日志
idf.py flash monitor
```

### Flash / PSRAM 规格（sdkconfig.defaults.esp32s3）

| 参数 | 值 |
|---|---|
| Flash | 16 MB QIO |
| PSRAM | 8 MB Octal 80 MHz |
| CPU | 240 MHz |
| 唤醒词模型 | `CONFIG_SR_WN_WN9_NIHAOXIAOZHI_TTS` |

---

## 使用

1. 上电后屏幕显示 **"Connecting WiFi..."**，连接成功后显示 **"Connected"** 并展示开心表情。
2. 说出唤醒词 **"你好小智"**，屏幕切换为 **"Listening..."** 状态。
3. 正常对话时，屏幕实时更新情绪表情和对话文本。
4. AI 可通过 MCP 工具主动调整音量、亮度、界面主题。

---

## 与上游的差异

| 变更点 | 说明 |
|---|---|
| BSP 替换 | 用 `components/bsp/` stub 替代 `esp_board_manager_adapter`，无需上游 BSP 库 |
| 无摄像头 | 本板无摄像头，已移除相关逻辑 |
| 音频 vol/gain | `esp_codec_dev_set_out_vol` / `set_in_gain` 失败记为警告而非错误 |

---

## 参考资料

- [2.8inch ESP32-S3 Display Wiki](https://www.lcdwiki.com/zh/2.8inch_ESP32-S3_Display)
- [espressif/esp-iot-solution xiaozhi_chat](https://github.com/espressif/esp-iot-solution/tree/master/examples/ai/xiaozhi_chat)
- [小智 AI 官网](https://xiaozhi.me)
