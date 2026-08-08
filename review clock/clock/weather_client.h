/**
 * @file    weather_client.h
 * @brief   OpenWeatherMap API 客户端
 *
 * 通过 WiFi 获取指定坐标的实时天气数据。
 * 解析 JSON 并缓存到结构体中, 调用方只读访问。
 *
 * 依赖: WiFi, HTTPClient, ArduinoJson
 */

#ifndef __WEATHER_CLIENT_H
#define __WEATHER_CLIENT_H

#include <stdint.h>

/* ========== 配置 ========== */
typedef struct {
    char  api_key[40];       /* OWM API Key */
    float lat;               /* 纬度 */
    float lon;               /* 经度 */
} Weather_Config;

/* ========== 状态 ========== */
typedef struct WeatherClient {
    /* --- 配置 --- */
    Weather_Config cfg;

    /* --- 缓存数据 --- */
    char temperature[8];     /* "23.5" */
    char description[32];    /* "晴" / "少云" ... */
    char icon[8];            /* "01d" / "09d" ... */
    char wind_speed[8];      /* "3.5" (m/s) */
    char wind_dir_cn[8];     /* "北风" / "东北风" ... */
    int  wind_deg;           /* 0~360 风向角度 (0=北) */
    char humidity[8];        /* "86" (%) */

    /* --- 状态 --- */
    uint8_t       online;    /* 是否成功获取过数据 */
    unsigned long last_update_ms;
} WeatherClient;

/* ========== API ========== */

/**
 * @brief  初始化天气客户端
 * @param  wc       客户端实例
 * @param  api_key  OpenWeatherMap API Key
 * @param  lat, lon 目标经纬度
 */
void Weather_Init(WeatherClient* wc, const char* api_key, float lat, float lon);

/**
 * @brief  从 API 获取天气数据 (HTTP GET)
 * @param  wc  客户端实例
 * @return 1=成功, 0=失败 (WiFi 断连/HTTP 错误/JSON 解析失败)
 */
int Weather_Update(WeatherClient* wc);

/**
 * @brief  运行时更新目标经纬度 (无需重新初始化)
 * @param  wc       客户端实例
 * @param  lat, lon 新经纬度
 */
void Weather_SetLocation(WeatherClient* wc, float lat, float lon);

/* ========== 只读访问器 ========== */
const char* Weather_GetTemp(const WeatherClient* wc);        /* "23.5" */
const char* Weather_GetDesc(const WeatherClient* wc);        /* "晴" */
const char* Weather_GetIcon(const WeatherClient* wc);        /* "01d" */
const char* Weather_GetWindSpeed(const WeatherClient* wc);   /* "3.5" (m/s) */
const char* Weather_GetWindDirCN(const WeatherClient* wc);   /* "北风" */
int         Weather_GetWindDeg(const WeatherClient* wc);     /* 0~360 */
const char* Weather_GetHumidity(const WeatherClient* wc);    /* "86" */
int         Weather_IsOnline(const WeatherClient* wc);       /* 是否获取过数据 */

#endif /* __WEATHER_CLIENT_H */
