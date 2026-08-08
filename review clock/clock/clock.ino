/**
 * @file    clock.ino
 * @brief   天气时钟 — ESP32 + SSD1306 OLED
 *
 * 模块化架构, 各子系统独立封装:
 *   - config_manager  : NVS 持久化 (WiFi 凭据 / API Key)
 *   - config.h        : 全局配置常量 + 默认值
 *   - chinese_font    : 中文字库 (16×16 点阵)
 *   - weather_icon    : OLED 动态天气图标
 *   - weather_client  : OpenWeatherMap API
 *   - time_manager    : NTP 时间同步
 *   - display_manager : OLED 画面布局
 *   - web_server      : HTTP 仪表盘 + WiFi 设置页
 *
 * 首次启动: SoftAP "WeatherClock" 配置门户 (192.168.4.1)
 * 后续启动: 自动从 NVS 加载凭据连接
 *
 * 硬件: ESP32 + SSD1306 128×64 I2C OLED
 */

#include <WiFi.h>
#include <Wire.h>
#include <DNSServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "config.h"
#include "config_manager.h"
#include "chinese_font.h"
#include "weather_icon.h"
#include "weather_client.h"
#include "time_manager.h"
#include "display_manager.h"
#include "web_server.h"

/* ========== 全局对象 ========== */

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

static TimeManager    timeMgr;
static WeatherClient  weatherMgr;
static DisplayManager displayMgr;
static WebManager     webMgr;
static ConfigManager  configMgr;
static DNSServer      dnsServer;

static unsigned long lastWeatherUpdate = 0;
static unsigned long lastDisplayRefresh = 0;
static bool          apActive = false;

/* ========== 城市变更回调 (网页切换 → 同步 OLED) ========== */
static void onCityChanged(const char* newCity)
{
    Display_SetCityName(&displayMgr, newCity);
    Display_Update(&displayMgr, &timeMgr, &weatherMgr);
}

/* ========== WiFi 连接 ========== */

static void startConfigAP(void)
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL);
    dnsServer.start(53, "*", WiFi.softAPIP());
    apActive = true;
    Display_ShowMultiMsg(&displayMgr, "WiFi Setup", WIFI_AP_SSID, "192.168.4.1");
}

static void wifiConnect(void)
{
    /* 无已保存凭据 → 进入配置门户 */
    if (!Config_HasWifi(&configMgr)) {
        startConfigAP();
        return;
    }

    /* 有凭据 → 尝试连接 */
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(configMgr.ssid, configMgr.pass);

    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < WIFI_RETRY_MAX) {
        delay(WIFI_RETRY_MS);
        retry++;
        if (retry % 10 == 0) {
            Display_ShowBootMsg(&displayMgr, "WiFi...");
        }
    }

    /* 连接失败 → AP 回落 (保留 STA 用于 loop 重试) */
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL);
        dnsServer.start(53, "*", WiFi.softAPIP());
        apActive = true;
        Display_ShowMultiMsg(&displayMgr, "WiFi Failed", WIFI_AP_SSID, "192.168.4.1");
    }
}

static void wifiCheck(void)
{
    if (WiFi.status() == WL_CONNECTED) {
        /* STA 已连接 → 关闭 AP */
        if (apActive) {
            dnsServer.stop();
            WiFi.softAPdisconnect(true);
            apActive = false;
            lastWeatherUpdate = 0;  /* 立即触发 NTP + 天气刷新 */
            Display_ShowBootMsg(&displayMgr, "Connected");
            /* 给一秒让显示稳定 */
            delay(1000);
        }
        return;
    }

    /* STA 断连 → AP 回落 + 定时重试 */
    if (Config_HasWifi(&configMgr)) {
        static unsigned long lastRetry = 0;
        if (millis() - lastRetry > WIFI_RETRY_AP_MS) {
            lastRetry = millis();
            if (!apActive) {
                WiFi.mode(WIFI_AP_STA);
                WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL);
                dnsServer.start(53, "*", WiFi.softAPIP());
                apActive = true;
                Display_ShowMultiMsg(&displayMgr, "WiFi Lost", WIFI_AP_SSID, "192.168.4.1");
            }
            WiFi.reconnect();
        }
    }
}

/** /savewifi 回调: 用新凭据重连并保持 AP 可访问 */
static void wifiReconnectFromConfig(void)
{
    if (!Config_HasWifi(&configMgr)) return;

    if (!apActive) {
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL);
        dnsServer.start(53, "*", WiFi.softAPIP());
        apActive = true;
    }
    WiFi.begin(configMgr.ssid, configMgr.pass);
}

/* ========== 初始化 ========== */

void setup()
{
    Serial.begin(115200);

    /* 1. OLED 硬件初始化 */
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("OLED init failed");
        for (;;);
    }
    Display_Init(&displayMgr, &display, DEFAULT_CITY);
    Display_ShowBootMsg(&displayMgr, "Booting...");

    /* 2. 加载配置 + WiFi 连接 */
    Config_Load(&configMgr);
    wifiConnect();

    Serial.println(WiFi.status() == WL_CONNECTED
        ? "WiFi OK"
        : (apActive ? "WiFi AP Mode" : "WiFi FAILED"));
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(WiFi.localIP());
    }

    /* 3. 模块初始化 */
    Time_Init(&timeMgr);
    Weather_Init(&weatherMgr, configMgr.api_key, SHEYANG_LAT, SHEYANG_LON);

    /* 4. Web 服务器 (同时监听 STA + SoftAP) */
    Web_Init(&webMgr, NULL, &timeMgr, &weatherMgr, &configMgr, DEFAULT_CITY);
    webMgr.on_city_change = onCityChanged;
    webMgr.on_wifi_save   = wifiReconnectFromConfig;

    /* 5. NTP + 天气 (仅 WiFi 已连接时) */
    if (WiFi.status() == WL_CONNECTED) {
        Display_ShowBootMsg(&displayMgr, "NTP...");
        Time_Sync(&timeMgr);
        if (Time_IsSynced(&timeMgr)) {
            Display_Update(&displayMgr, &timeMgr, &weatherMgr);
        }
        Weather_Update(&weatherMgr);
        Display_Update(&displayMgr, &timeMgr, &weatherMgr);
        lastWeatherUpdate = millis();
    } else {
        lastWeatherUpdate = 0;
    }
}

/* ========== 主循环 ========== */

void loop()
{
    Web_HandleClient(&webMgr);

    /* DNS 劫持 (AP 模式) */
    if (apActive) {
        dnsServer.processNextRequest();
    }

    wifiCheck();

    /* 定时更新天气 + NTP (AP 模式下跳过) */
    if (!apActive && millis() - lastWeatherUpdate > WEATHER_UPDATE_MS) {
        Time_Sync(&timeMgr);
        Weather_Update(&weatherMgr);
        Display_Update(&displayMgr, &timeMgr, &weatherMgr);
        lastWeatherUpdate = millis();
    }

    /* 每秒刷新显示 (AP 模式下跳过, 保留配置屏) */
    if (!apActive && millis() - lastDisplayRefresh > DISPLAY_REFRESH_MS) {
        Time_Refresh(&timeMgr);
        Display_Update(&displayMgr, &timeMgr, &weatherMgr);
        lastDisplayRefresh = millis();
    }

    delay(LOOP_DELAY_MS);
}
