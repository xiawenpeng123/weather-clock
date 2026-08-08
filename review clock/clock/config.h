/**
 * @file    config.h
 * @brief   全局配置常量 — WiFi / 屏幕 / API / 定时参数
 *
 * WiFi 凭据通过首次启动的 SoftAP 配置门户输入, 持久化到 NVS。
 * OWM_API_KEY 为默认值, 可在 /wifi 页面覆盖。开源前请替换为占位符。
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/* ========== OLED 屏幕 ========== */
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_ADDR      0x3C    /* I2C 地址, 通常 0x3C 或 0x3D */

/* ========== WiFi ========== */
#define WIFI_RETRY_MAX      40       /* STA 启动时最大重试次数 */
#define WIFI_RETRY_MS       500      /* 每次重试间隔 (ms) */
#define WIFI_AP_SSID        "WeatherClock"  /* 配置门户 SoftAP SSID */
#define WIFI_AP_PASSWORD    ""       /* 空 = 开放 AP (无密码) */
#define WIFI_AP_CHANNEL     1
#define WIFI_RETRY_AP_MS    30000    /* STA 断连后重试间隔 (ms) */

/* ========== OpenWeatherMap ========== */
#define OWM_API_KEY     "881b0510ece6a5d8fe5eedb841725e8f"  /* 默认值, /wifi 页可覆盖 */
#define SHEYANG_LAT     33.78f  /* 默认纬度 */
#define SHEYANG_LON     120.25f /* 默认经度 */
#define DEFAULT_CITY    "射阳"  /* 默认城市名 */

/* ========== 定时参数 ========== */
#define WEATHER_UPDATE_MS    600000   /* 天气更新间隔 (10 分钟) */
#define DISPLAY_REFRESH_MS   1000     /* 显示刷新间隔 (1 秒) */
#define LOOP_DELAY_MS        100      /* 主循环 delay (ms) */

#endif /* __CONFIG_H */
