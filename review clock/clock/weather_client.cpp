/**
 * @file    weather_client.cpp
 * @brief   OpenWeatherMap API 客户端 实现 (无 String 依赖)
 *
 * 数据流: WiFi → HTTP GET → JSON 解析 → 结构体缓存
 * 使用 char[] 代替 String 以减少 flash 占用。
 *
 * 依赖: WiFi, HTTPClient, ArduinoJson
 */

#include "weather_client.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

/* ========== 可配置参数 ========== */
#define WEATHER_HTTP_TIMEOUT_MS  5000   /* HTTP 请求超时 (ms) */

/* URL 缓冲区 (足够容纳完整 API URL) */
#define URL_BUF_SIZE  192

void Weather_Init(WeatherClient* wc, const char* api_key, float lat, float lon)
{
    if (!wc) return;
    strncpy(wc->cfg.api_key, api_key, sizeof(wc->cfg.api_key) - 1);
    wc->cfg.api_key[sizeof(wc->cfg.api_key) - 1] = '\0';
    wc->cfg.lat = lat;
    wc->cfg.lon = lon;
    wc->temperature[0] = '\0';
    wc->description[0] = '\0';
    wc->icon[0] = '\0';
    wc->wind_speed[0] = '\0';
    wc->wind_dir_cn[0] = '\0';
    wc->wind_deg = 0;
    wc->humidity[0] = '\0';
    wc->online = 0;
    wc->last_update_ms = 0;
}

int Weather_Update(WeatherClient* wc)
{
    if (!wc) return 0;
    if (WiFi.status() != WL_CONNECTED) return 0;

    HTTPClient http;
    http.setTimeout(WEATHER_HTTP_TIMEOUT_MS);

    /* 用 snprintf 构造 URL, 避免 String 拼接 */
    char url[URL_BUF_SIZE];
    snprintf(url, sizeof(url),
        "http://api.openweathermap.org/data/2.5/weather?lat=%.4f&lon=%.4f&appid=%s&units=metric&lang=zh_cn",
        wc->cfg.lat, wc->cfg.lon, wc->cfg.api_key);

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        http.end();
        return 0;
    }

    /* 使用 getString() 获取响应 (一次性, 用完即释放) */
    String payload = http.getString();
    http.end();

    /* 减小 JSON 文档: 512 字节足够解析 OWM 当前天气 */
    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) return 0;

    float temp = doc["main"]["temp"].as<float>();
    snprintf(wc->temperature, sizeof(wc->temperature), "%.1f", temp);

    const char* desc = doc["weather"][0]["description"].as<const char*>();
    strncpy(wc->description, desc ? desc : "", sizeof(wc->description) - 1);
    wc->description[sizeof(wc->description) - 1] = '\0';

    const char* ic = doc["weather"][0]["icon"].as<const char*>();
    strncpy(wc->icon, ic ? ic : "", sizeof(wc->icon) - 1);
    wc->icon[sizeof(wc->icon) - 1] = '\0';

    /* 风速 */
    float wSpeed = doc["wind"]["speed"].as<float>();
    snprintf(wc->wind_speed, sizeof(wc->wind_speed), "%.1f", wSpeed);

    /* 风向角度 */
    int wDeg = doc["wind"]["deg"].as<int>();
    wc->wind_deg = wDeg;

    /* 风向角度 → 中文 (8 方位) */
    static const char* dirTable[] = {"北","东北","东","东南","南","西南","西","西北"};
    int dirIdx = ((wDeg + 22) % 360) / 45;
    snprintf(wc->wind_dir_cn, sizeof(wc->wind_dir_cn), "%s风", dirTable[dirIdx]);

    /* 湿度 */
    int hum = doc["main"]["humidity"].as<int>();
    snprintf(wc->humidity, sizeof(wc->humidity), "%d", hum);

    wc->online = 1;
    wc->last_update_ms = millis();
    return 1;
}

const char* Weather_GetTemp(const WeatherClient* wc)
{
    return (wc && wc->temperature[0]) ? wc->temperature : "--.-";
}

const char* Weather_GetDesc(const WeatherClient* wc)
{
    return (wc && wc->description[0]) ? wc->description : "loading";
}

const char* Weather_GetIcon(const WeatherClient* wc)
{
    return (wc && wc->icon[0]) ? wc->icon : "01d";
}

const char* Weather_GetWindSpeed(const WeatherClient* wc)
{
    return (wc && wc->wind_speed[0]) ? wc->wind_speed : "--";
}

const char* Weather_GetWindDirCN(const WeatherClient* wc)
{
    return (wc && wc->wind_dir_cn[0]) ? wc->wind_dir_cn : "--";
}

int Weather_GetWindDeg(const WeatherClient* wc)
{
    return wc ? wc->wind_deg : 0;
}

const char* Weather_GetHumidity(const WeatherClient* wc)
{
    return (wc && wc->humidity[0]) ? wc->humidity : "--";
}

int Weather_IsOnline(const WeatherClient* wc)
{
    return wc ? wc->online : 0;
}

void Weather_SetLocation(WeatherClient* wc, float lat, float lon)
{
    if (!wc) return;
    wc->cfg.lat = lat;
    wc->cfg.lon = lon;
}
