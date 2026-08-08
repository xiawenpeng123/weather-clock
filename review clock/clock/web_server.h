/**
 * @file    web_server.h
 * @brief   天气时钟 Web 页面服务器
 *
 * 在 ESP32 的 80 端口提供仪表盘 + WiFi 配置页:
 *   - /        暗色主题天气时钟仪表盘
 *   - /setcity POST 切换城市 (Geocoding API)
 *   - /wifi     WiFi 设置页 (SSID/密码/API Key)
 *   - /savewifi POST 保存 WiFi 配置到 NVS
 *
 * 同时监听 STA 和 SoftAP 接口 (192.168.4.1)。
 *
 * 依赖: WebServer, TimeManager, WeatherClient, ConfigManager
 */

#ifndef __WEB_SERVER_H
#define __WEB_SERVER_H

#include <WebServer.h>
#include <stdint.h>
#include "config_manager.h"

/* 前向声明 */
struct TimeManager;
struct WeatherClient;

/* ========== 配置 ========== */
typedef struct {
    uint16_t port;           /* HTTP 端口 (默认 80) */
} Web_Config;

#define WEB_DEFAULT_CONFIG   { .port = 80 }
#define WEB_CITY_NAME_MAX    32

/* 回调 */
typedef void (*CityChangeCallback)(const char* new_city);
typedef void (*WifiSaveCallback)(void);

/* ========== 状态 ========== */
typedef struct {
    WebServer*          server;
    const TimeManager*  time;
    WeatherClient*      weather;
    char                city_name[WEB_CITY_NAME_MAX];
    CityChangeCallback  on_city_change;
    ConfigManager*      cfg;            /* 运行时配置 (可读写) */
    WifiSaveCallback    on_wifi_save;   /* /savewifi 后触发重连 */
} WebManager;

/* ========== API ========== */

void Web_Init(WebManager* wm, const Web_Config* cfg,
              const TimeManager* tm, WeatherClient* wc,
              ConfigManager* cm, const char* city_name);

void Web_HandleClient(WebManager* wm);

#endif /* __WEB_SERVER_H */
