# ESP32 天气时钟

基于 ESP32-WROOM-32 + 0.96 寸 OLED 的网络天气时钟，支持 OLED 显示和 Web 网页两种查看方式。

---

## 硬件清单

| 名称 | 型号 | 数量 |
|------|------|------|
| 主控 | ESP32-WROOM-32 开发板 | 1 |
| 显示屏 | 0.96 寸 OLED（SSD1306 驱动，I2C 接口，128×64） | 1 |
| 连接线 | 杜邦线 母对母 ×4 | 4 |
| 数据线 | Micro USB 数据线 | 1 |

---

## 接线方式

OLED 为 I2C 接口（4 个引脚），与 ESP32 连接如下：

```
OLED          ESP32
─────         ─────
VCC    ───    3.3V
GND    ───    GND
SCL    ───    GPIO 22
SDA    ───    GPIO 21
```

> 引脚对照表：

| OLED 引脚 | ESP32 引脚 | 说明 |
|-----------|------------|------|
| VCC | 3.3V | 电源正极（注意是 3.3V 不是 5V） |
| GND | GND | 电源负极 |
| SCL | GPIO 22 | I2C 时钟线 |
| SDA | GPIO 21 | I2C 数据线 |

> ⚠️ 注意：OLED 的 VCC 接 **3.3V** 而非 5V，否则可能烧坏屏幕。

---

## 需要的 Arduino 库

在 Arduino IDE 的「库管理」中搜索并安装：

| 库名 | 作者 | 用途 |
|------|------|------|
| Adafruit SSD1306 | Adafruit | OLED 驱动 |
| Adafruit GFX Library | Adafruit | 图形显示 |
| ArduinoJson | Benoit Blanchon | JSON 解析 |

> ESP32 开发板支持：在 Arduino IDE 中安装 **esp32 by Espressif Systems**（文件 → 首选项 → 附加开发板管理器网址：`https://espressif.github.io/arduino-esp32/package_esp32_index.json`）

---

## 功能说明

| 功能 | 说明 |
|------|------|
| ⏰ NTP 授时 | 自动从阿里云/腾讯 NTP 服务器获取北京时间（UTC+8） |
| 🌤️ 天气显示 | 调用 OpenWeatherMap API，每 60 秒刷新 |
| 📟 OLED 显示 | 128×64 屏幕四行布局：日期、时间、天气、温度城市 |
| 🌐 Web 页面 | 手机/电脑浏览器访问 ESP32 的 IP 即可查看（深色 UI，10 秒自动刷新） |
| 🔄 WiFi 断线重连 | 自动检测并重新连接 |

### OLED 屏幕布局

```
┌──────────────────────────────┐
│     Wed  2025-08-06          │  ← 星期 + 日期
│          15:30               │  ← 时间（大字）
│ ──────────────────────       │  ← 分隔线
│     * Clear Sky              │  ← 天气图标 + 描述
│     25.3C  |  Yancheng       │  ← 温度 + 城市
└──────────────────────────────┘
```

---

## 使用前配置

打开 `clock.ino`，修改以下三项为你自己的信息：

```cpp
// 1. WiFi
const char* ssid = "你的WiFi名称";
const char* password = "你的WiFi密码";

// 2. 城市（英文名）
String city = "yancheng";   // 改为你的城市，如 "beijing", "shanghai"

// 3. OpenWeatherMap API Key（免费注册 https://openweathermap.org/api）
String apiKey = "你的API_KEY";
```

> 免费 API Key 注册地址：https://home.openweathermap.org/users/sign_up

---

## 使用方法

1. 按接线表连好硬件
2. USB 数据线连接 ESP32 到电脑
3. Arduino IDE 选择开发板 **ESP32 Dev Module**，端口选对应 COM 口
4. 打开 `clock.ino` → 编译 → 烧录
5. 烧录后打开 **串口监视器**（115200 波特率），查看 ESP32 分配的 IP 地址
6. 手机/电脑浏览器访问 `http://<IP地址>` 即可看到 Web 页面

---

## 项目文件

```
clock/
├── clock.ino      # 主程序
└── README.md      # 本说明文档
```
