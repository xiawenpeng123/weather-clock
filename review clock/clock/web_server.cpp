/**
 * @file    web_server.cpp
 * @brief   Web 仪表盘页面 实现 (无 String 拼接)
 *
 * 在 80 端口提供仪表盘 + WiFi 配置页:
 *   - /         暗色主题天气时钟仪表盘 + 城市切换
 *   - /setcity  切换城市 (Geocoding API)
 *   - /wifi     WiFi 设置页 (SSID/密码/API Key)
 *   - /savewifi 保存 WiFi 配置到 NVS
 *
 * HTML 通过 snprintf 一次性构建到静态缓冲区, 避免 String 拼接。
 *
 * 依赖: WebServer, TimeManager, WeatherClient, ConfigManager
 */

#include "web_server.h"
#include "time_manager.h"
#include "weather_client.h"
#include "config_manager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <string.h>

/* ========== 页面缓冲区 ========== */
/* 预分配静态缓冲区, 避免 String 动态内存分配 */
#define HTML_BUF_SIZE  4096
static char htmlBuf[HTML_BUF_SIZE];

/* ========== 内部辅助 ========== */

/** OWM icon 代码 → Web 页面 emoji */
static const char* weatherEmoji(const char* code)
{
    if (!code || code[0] == '\0' || code[1] == '\0') return "";
    if (code[0] == '0' && code[1] == '1') return "☀️";
    if (code[0] == '0' && code[1] == '2') return "🌤️";
    if (code[0] == '0' && code[1] == '3') return "⛅";
    if (code[0] == '0' && code[1] == '4') return "☁️";
    if (code[0] == '0' && code[1] == '9') return "🌧️";
    if (code[0] == '1' && code[1] == '0') return "🌦️";
    if (code[0] == '1' && code[1] == '1') return "⛈️";
    if (code[0] == '1' && code[1] == '3') return "❄️";
    if (code[0] == '5' && code[1] == '0') return "🌫️";
    return "";
}

/**
 * @brief  对查询参数值进行 URL 编码 (RFC 3986 保守编码)
 *
 * 字母数字和 -_.~ 保留原样, 空格 → %20, 其余字节 → %XX。
 * 中文等多字节 UTF-8 字符的每个字节都会被编码。
 */
static void urlEncodeParam(const char* src, char* dst, size_t dstSize)
{
    if (!src || !dst || dstSize == 0) return;
    size_t di = 0;
    for (const char* p = src; *p && di + 4 < dstSize; p++) {
        unsigned char c = (unsigned char)*p;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' ||
            c == '.' || c == '~') {
            dst[di++] = (char)c;
        } else if (c == ' ') {
            dst[di++] = '%';
            dst[di++] = '2';
            dst[di++] = '0';
        } else {
            static const char hex[] = "0123456789ABCDEF";
            dst[di++] = '%';
            dst[di++] = hex[c >> 4];
            dst[di++] = hex[c & 0x0F];
        }
    }
    dst[di] = '\0';
}

/**
 * @brief  通过 OWM Geocoding API 将城市名解析为经纬度
 * @param  api_key    OWM API Key
 * @param  city_name  城市名 (UTF-8, 如 "北京" / "Shanghai")
 * @param  out_lat    输出: 纬度
 * @param  out_lon    输出: 经度
 * @param  out_name   输出: API 返回的标准城市名 (可传 NULL)
 * @param  name_size  out_name 缓冲区大小
 * @return 1=成功, 0=失败
 */
static int geocodeCity(const char* api_key, const char* city_name,
                       float* out_lat, float* out_lon,
                       char* out_name, size_t name_size)
{
    if (!api_key || !city_name || !out_lat || !out_lon) return 0;
    if (WiFi.status() != WL_CONNECTED) return 0;

    /* URL 编码城市名 */
    char encCity[128];
    urlEncodeParam(city_name, encCity, sizeof(encCity));

    HTTPClient http;
    http.setTimeout(5000);

    char url[384];
    snprintf(url, sizeof(url),
        "http://api.openweathermap.org/geo/1.0/direct?q=%s&limit=1&appid=%s",
        encCity, api_key);

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        http.end();
        return 0;
    }

    String payload = http.getString();
    http.end();

    /* 解析 JSON 数组 */
    StaticJsonDocument<768> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) return 0;

    if (doc.is<JsonArray>() && doc.size() > 0) {
        *out_lat = doc[0]["lat"].as<float>();
        *out_lon = doc[0]["lon"].as<float>();

        /* 优先取中文名, 否则取英文名 */
        if (out_name && name_size > 0) {
            const char* localZh = doc[0]["local_names"]["zh"];
            const char* name    = doc[0]["name"];
            const char* best    = (localZh && localZh[0]) ? localZh : name;
            if (best) {
                strncpy(out_name, best, name_size - 1);
                out_name[name_size - 1] = '\0';
            } else {
                out_name[0] = '\0';
            }
        }
        return 1;
    }

    return 0;
}

/* ========== 路由处理 ========== */

static void handleRoot(WebManager* wm)
{
    if (!wm) return;

    /* 用 snprintf 一次性构建 HTML, 零 String 开销 */
    snprintf(htmlBuf, sizeof(htmlBuf),
        "<!DOCTYPE html><html lang=\"zh-CN\"><head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">"
        "<meta http-equiv=\"refresh\" content=\"10\">"
        "<title>Weather Clock - %s</title>"
        "<style>"
        "*{margin:0;padding:0;box-sizing:border-box}"
        "body{font-family:system-ui,sans-serif;min-height:100vh;"
        "display:flex;justify-content:center;align-items:center;"
        "background:linear-gradient(135deg,#0c0f1e,#1a1d33,#0f1a2e);color:#e8eaed}"
        ".card{background:rgba(255,255,255,.04);border:1px solid rgba(255,255,255,.08);"
        "border-radius:24px;padding:40px 48px;text-align:center;"
        "box-shadow:0 20px 60px rgba(0,0,0,.4);min-width:340px}"
        ".time{font-size:80px;font-weight:300;letter-spacing:4px;color:#fff;"
        "line-height:1;margin-bottom:8px;font-variant-numeric:tabular-nums}"
        ".date{font-size:18px;color:#9aa0b4;letter-spacing:1px;margin-bottom:24px}"
        ".divider{width:60px;height:2px;background:linear-gradient(90deg,transparent,#4a6cf7,transparent);"
        "margin:0 auto 24px;border-radius:1px}"
        ".w-row{display:flex;align-items:center;justify-content:center;gap:12px;margin-bottom:8px}"
        ".w-emoji{font-size:36px;line-height:1}"
        ".w-desc{font-size:20px;color:#c4c9d4}"
        ".i-row{display:flex;align-items:center;justify-content:center;gap:16px;margin-top:4px}"
        ".temp{font-size:28px;font-weight:500;color:#fff}"
        ".city{font-size:16px;color:#7c8299}"
        ".city-form{margin-top:22px;display:flex;align-items:center;"
        "justify-content:center;gap:8px;flex-wrap:wrap}"
        ".city-input{background:rgba(255,255,255,.06);border:1px solid rgba(255,255,255,.15);"
        "border-radius:8px;padding:8px 12px;color:#e8eaed;font-size:14px;width:160px;"
        "outline:none;text-align:center;transition:border-color .2s}"
        ".city-input:focus{border-color:#4a6cf7}"
        ".city-input::placeholder{color:#5a6078}"
        ".city-btn{background:#4a6cf7;border:none;border-radius:8px;padding:8px 18px;"
        "color:#fff;font-size:14px;cursor:pointer;font-weight:500;"
        "transition:background .2s}"
        ".city-btn:hover{background:#5b7bf8}"
        ".city-btn:active{background:#3a5ce5}"
        ".footer{margin-top:22px;font-size:12px;color:#555a6e}"
        ".dot{display:inline-block;width:6px;height:6px;background:#4ade80;"
        "border-radius:50%%;margin-right:6px;animation:pulse 2s infinite}"
        "@keyframes pulse{0%%,100%%{opacity:1}50%%{opacity:.3}}"
        "</style></head><body>"
        "<div class=\"card\">"
        "<div class=\"time\">%s</div>"
        "<div class=\"date\">%s  %s</div>"
        "<div class=\"divider\"></div>"
        "<div class=\"w-row\">"
        "<span class=\"w-emoji\">%s</span>"
        "<span class=\"w-desc\">%s</span>"
        "</div>"
        "<div class=\"i-row\">"
        "<span class=\"temp\">%s°C</span>"
        "<span class=\"city\">%s</span>"
        "</div>"
        "<form class=\"city-form\" method=\"POST\" action=\"/setcity\">"
        "<input class=\"city-input\" type=\"text\" name=\"city\" "
        "placeholder=\"输入城市名, 如: 北京\" autocomplete=\"off\">"
        "<button class=\"city-btn\" type=\"submit\">切换城市</button>"
        "</form>"
        "<div class=\"footer\"><a href=\"/wifi\" style=\"color:#7c8299;text-decoration:none;\">⚙ WiFi</a>  ·  <span class=\"dot\"></span>Live  ·  auto-refresh</div>"
        "</div></body></html>",
        wm->city_name[0] ? wm->city_name : "Weather",
        Time_GetTimeStr(wm->time),
        Time_GetWeekdayStr(wm->time), Time_GetDateStr(wm->time),
        weatherEmoji(Weather_GetIcon(wm->weather)),
        Weather_GetDesc(wm->weather),
        Weather_GetTemp(wm->weather),
        wm->city_name[0] ? wm->city_name : "--"
    );

    wm->server->send(200, "text/html", htmlBuf);
}

/**
 * @brief  剥离地名末尾的行政区划后缀 (市/县/区/州/省/盟/旗/自治县 等)
 *
 * 原地修改, 将后缀替换为 '\0'。
 * 支持的后缀: 自治县, 自治州, 自治旗, 林区, 特区, 新区,
 *             市, 县, 区, 省, 州, 盟, 旗, 地区
 */
static void stripAdminSuffix(char* name)
{
    if (!name || name[0] == '\0') return;
    size_t len = strlen(name);

    /* 3+ 字后缀 (按长度从长到短匹配) */
    if (len >= 9 && strcmp(name + len - 9, "自治县") == 0) { name[len - 9] = '\0'; return; }
    if (len >= 9 && strcmp(name + len - 9, "自治州") == 0) { name[len - 9] = '\0'; return; }
    if (len >= 9 && strcmp(name + len - 9, "自治旗") == 0) { name[len - 9] = '\0'; return; }
    if (len >= 6 && strcmp(name + len - 6, "地区") == 0)   { name[len - 6] = '\0'; return; }
    if (len >= 6 && strcmp(name + len - 6, "林区") == 0)   { name[len - 6] = '\0'; return; }
    if (len >= 6 && strcmp(name + len - 6, "特区") == 0)   { name[len - 6] = '\0'; return; }
    if (len >= 6 && strcmp(name + len - 6, "新区") == 0)   { name[len - 6] = '\0'; return; }

    /* 单字后缀 */
    if (len >= 3) {
        const char* last3 = name + len - 3;
        if (strcmp(last3, "市") == 0 || strcmp(last3, "县") == 0 ||
            strcmp(last3, "区") == 0 || strcmp(last3, "省") == 0 ||
            strcmp(last3, "州") == 0 || strcmp(last3, "盟") == 0 ||
            strcmp(last3, "旗") == 0) {
            name[len - 3] = '\0';
        }
    }
}

/**
 * @brief  处理 /setcity POST 请求
 *
 * 流程: 接收城市名 → Geocoding API 解析坐标 → 剥离行政后缀 →
 *       更新 WeatherClient → 立即拉取天气 → 更新 city_name →
 *       通知 OLED (回调) → 重定向回 /
 */
static void handleSetCity(WebManager* wm)
{
    if (!wm || !wm->server) return;

    String city = wm->server->arg("city");
    if (city.length() == 0) {
        wm->server->sendHeader("Location", "/", true);
        wm->server->send(302, "text/plain", "");
        return;
    }

    /* Geocoding: 城市名 → 经纬度 */
    float newLat = 0, newLon = 0;
    char resolvedName[WEB_CITY_NAME_MAX];
    resolvedName[0] = '\0';

    int geoOk = geocodeCity(wm->cfg ? wm->cfg->api_key : wm->weather->cfg.api_key, city.c_str(),
                            &newLat, &newLon,
                            resolvedName, sizeof(resolvedName));

    if (!geoOk) {
        /* Geocoding 失败 — 返回提示页面 */
        snprintf(htmlBuf, sizeof(htmlBuf),
            "<!DOCTYPE html><html lang=\"zh-CN\"><head>"
            "<meta charset=\"UTF-8\">"
            "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">"
            "<meta http-equiv=\"refresh\" content=\"3;url=/\">"
            "<title>City Not Found</title>"
            "<style>"
            "*{margin:0;padding:0;box-sizing:border-box}"
            "body{font-family:system-ui,sans-serif;min-height:100vh;"
            "display:flex;justify-content:center;align-items:center;"
            "background:linear-gradient(135deg,#0c0f1e,#1a1d33,#0f1a2e);color:#e8eaed}"
            ".card{background:rgba(255,255,255,.04);border:1px solid rgba(255,255,255,.08);"
            "border-radius:24px;padding:40px 48px;text-align:center;"
            "box-shadow:0 20px 60px rgba(0,0,0,.4)}"
            ".err{font-size:48px;margin-bottom:12px}"
            ".msg{font-size:18px;color:#9aa0b4}"
            ".hint{font-size:13px;color:#555a6e;margin-top:10px}"
            "</style></head><body>"
            "<div class=\"card\">"
            "<div class=\"err\">🔍</div>"
            "<div class=\"msg\">未找到城市 \"%s\"</div>"
            "<div class=\"hint\">3 秒后自动返回…</div>"
            "</div></body></html>",
            city.c_str());
        wm->server->send(404, "text/html", htmlBuf);
        return;
    }

    /* 用 API 返回的标准名; 如果没取到则用用户输入 */
    char finalName[WEB_CITY_NAME_MAX];
    if (resolvedName[0] != '\0') {
        strncpy(finalName, resolvedName, WEB_CITY_NAME_MAX - 1);
        finalName[WEB_CITY_NAME_MAX - 1] = '\0';
    } else {
        strncpy(finalName, city.c_str(), WEB_CITY_NAME_MAX - 1);
        finalName[WEB_CITY_NAME_MAX - 1] = '\0';
    }
    stripAdminSuffix(finalName);  /* 去掉末尾的 市/县/区/州 等行政后缀 */

    /* 更新天气客户端: 新坐标 + 立即拉取 */
    Weather_SetLocation(wm->weather, newLat, newLon);
    Weather_Update(wm->weather);

    /* 更新城市名 */
    strncpy(wm->city_name, finalName, WEB_CITY_NAME_MAX - 1);
    wm->city_name[WEB_CITY_NAME_MAX - 1] = '\0';

    /* 通知 OLED 同步城市名 */
    if (wm->on_city_change) {
        wm->on_city_change(wm->city_name);
    }

    /* 重定向回首页 */
    wm->server->sendHeader("Location", "/", true);
    wm->server->send(302, "text/plain", "");
}

/* ========== WiFi 设置页 ========== */

static void handleWifi(WebManager* wm)
{
    if (!wm || !wm->cfg) return;

    const char* ipStr = "N/A";
    if (WiFi.status() == WL_CONNECTED) {
        ipStr = WiFi.localIP().toString().c_str();
    } else {
        IPAddress apIP = WiFi.softAPIP();
        if (apIP.toString().length() > 0) {
            ipStr = apIP.toString().c_str();
        }
    }

    snprintf(htmlBuf, sizeof(htmlBuf),
        "<!DOCTYPE html><html lang=\"zh-CN\"><head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">"
        "<title>WiFi Settings</title>"
        "<style>"
        "*{margin:0;padding:0;box-sizing:border-box}"
        "body{font-family:system-ui,sans-serif;min-height:100vh;"
        "display:flex;justify-content:center;align-items:center;"
        "background:linear-gradient(135deg,#0c0f1e,#1a1d33,#0f1a2e);color:#e8eaed}"
        ".card{background:rgba(255,255,255,.04);border:1px solid rgba(255,255,255,.08);"
        "border-radius:24px;padding:36px 40px;text-align:center;"
        "box-shadow:0 20px 60px rgba(0,0,0,.4);min-width:320px;max-width:400px}"
        "h2{font-size:22px;font-weight:400;color:#fff;margin-bottom:6px}"
        ".sub{font-size:13px;color:#7c8299;margin-bottom:20px}"
        "label{display:block;text-align:left;font-size:13px;color:#9aa0b4;"
        "margin-bottom:4px;margin-top:14px}"
        "input{width:100%%;background:rgba(255,255,255,.06);"
        "border:1px solid rgba(255,255,255,.15);border-radius:8px;"
        "padding:10px 14px;color:#e8eaed;font-size:14px;outline:none;"
        "transition:border-color .2s}"
        "input:focus{border-color:#4a6cf7}"
        "input::placeholder{color:#5a6078}"
        ".btn{width:100%%;margin-top:20px;background:#4a6cf7;border:none;"
        "border-radius:8px;padding:12px;color:#fff;font-size:15px;"
        "cursor:pointer;font-weight:500;transition:background .2s}"
        ".btn:hover{background:#5b7bf8}"
        ".back{margin-top:14px;font-size:13px}"
        ".back a{color:#7c8299;text-decoration:none}"
        ".back a:hover{color:#aab0c0}"
        ".note{font-size:11px;color:#555a6e;margin-top:16px;text-align:left;"
        "line-height:1.6}"
        "</style></head><body>"
        "<div class=\"card\">"
        "<h2>⚙ WiFi 设置</h2>"
        "<div class=\"sub\">当前 IP: %s</div>"
        "<form method=\"POST\" action=\"/savewifi\">"
        "<label>WiFi 名称 (SSID)</label>"
        "<input type=\"text\" name=\"ssid\" value=\"%s\" required "
        "placeholder=\"输入 WiFi 名称\">"
        "<label>WiFi 密码</label>"
        "<input type=\"password\" name=\"pass\" placeholder=\"%s\">"
        "<label>OWM API Key</label>"
        "<input type=\"text\" name=\"apikey\" value=\"%s\" "
        "placeholder=\"OpenWeatherMap API Key\">"
        "<button class=\"btn\" type=\"submit\">保存并重连</button>"
        "</form>"
        "<div class=\"back\"><a href=\"/\">← 返回仪表盘</a></div>"
        "<div class=\"note\">"
        "密码留空则保留当前密码。<br>"
        "API Key 用于天气查询, 留空则使用默认值。<br>"
        "如需重置所有设置, 请擦除 ESP32 Flash。"
        "</div>"
        "</div></body></html>",
        ipStr,
        wm->cfg->ssid,
        wm->cfg->pass[0] ? "留空则保留当前密码" : "请输入 WiFi 密码",
        wm->cfg->api_key
    );

    wm->server->send(200, "text/html", htmlBuf);
}

static void handleSaveWifi(WebManager* wm)
{
    if (!wm || !wm->cfg) return;

    String ssidArg = wm->server->arg("ssid");
    String passArg = wm->server->arg("pass");
    String keyArg  = wm->server->arg("apikey");

    if (ssidArg.length() == 0) {
        wm->server->sendHeader("Location", "/wifi", true);
        wm->server->send(302, "text/plain", "");
        return;
    }

    /* 拒绝首次设置时填空白密码 */
    if (passArg.length() == 0 && wm->cfg->pass[0] == '\0') {
        snprintf(htmlBuf, sizeof(htmlBuf),
            "<!DOCTYPE html><html lang=\"zh-CN\"><head>"
            "<meta charset=\"UTF-8\"><meta http-equiv=\"refresh\" content=\"2;url=/wifi\">"
            "<title>Error</title><style>"
            "body{font-family:system-ui,sans-serif;min-height:100vh;"
            "display:flex;justify-content:center;align-items:center;"
            "background:#0c0f1e;color:#e8eaed}"
            ".card{background:rgba(255,255,255,.04);border-radius:24px;padding:40px;text-align:center}"
            "</style></head><body>"
            "<div class=\"card\"><h2>⚠️</h2><p>首次使用需要输入 WiFi 密码</p></div>"
            "</body></html>");
        wm->server->send(400, "text/html", htmlBuf);
        return;
    }

    /* 写入配置 */
    strncpy(wm->cfg->ssid, ssidArg.c_str(), CFG_SSID_MAX - 1);
    wm->cfg->ssid[CFG_SSID_MAX - 1] = '\0';

    if (passArg.length() > 0) {
        strncpy(wm->cfg->pass, passArg.c_str(), CFG_PASS_MAX - 1);
        wm->cfg->pass[CFG_PASS_MAX - 1] = '\0';
    }

    if (keyArg.length() > 0) {
        strncpy(wm->cfg->api_key, keyArg.c_str(), CFG_APIKEY_MAX - 1);
        wm->cfg->api_key[CFG_APIKEY_MAX - 1] = '\0';
    }

    Config_SaveWifi(wm->cfg);
    Config_SaveApiKey(wm->cfg);

    /* 同步 API key 到天气客户端 */
    strncpy(wm->weather->cfg.api_key, wm->cfg->api_key,
            sizeof(wm->weather->cfg.api_key) - 1);
    wm->weather->cfg.api_key[sizeof(wm->weather->cfg.api_key) - 1] = '\0';

    /* 触发 WiFi 重连 */
    if (wm->on_wifi_save) wm->on_wifi_save();

    wm->server->sendHeader("Location", "/wifi", true);
    wm->server->send(302, "text/plain", "");
}

/* ========== API ========== */

void Web_Init(WebManager* wm, const Web_Config* cfg,
              const TimeManager* tm, WeatherClient* wc,
              ConfigManager* cm, const char* city_name)
{
    if (!wm) return;

    uint16_t port = (cfg && cfg->port > 0) ? cfg->port : 80;
    wm->server         = new WebServer(port);
    wm->time           = tm;
    wm->weather        = wc;
    wm->cfg            = cm;
    wm->on_city_change = NULL;
    wm->on_wifi_save   = NULL;

    /* 复制城市名到自有缓冲区 */
    if (city_name) {
        strncpy(wm->city_name, city_name, WEB_CITY_NAME_MAX - 1);
        wm->city_name[WEB_CITY_NAME_MAX - 1] = '\0';
    } else {
        wm->city_name[0] = '\0';
    }

    /* 注册路由 */
    wm->server->on("/",          [wm]() { handleRoot(wm); });
    wm->server->on("/setcity",   HTTP_POST, [wm]() { handleSetCity(wm); });
    wm->server->on("/wifi",      [wm]() { handleWifi(wm); });
    wm->server->on("/savewifi",  HTTP_POST, [wm]() { handleSaveWifi(wm); });
    wm->server->begin();
}

void Web_HandleClient(WebManager* wm)
{
    if (wm && wm->server) {
        wm->server->handleClient();
    }
}
