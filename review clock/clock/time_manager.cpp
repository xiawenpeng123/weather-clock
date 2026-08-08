/**
 * @file    time_manager.cpp
 * @brief   NTP 时间管理器 实现
 *
 * 依次尝试 3 个 NTP 服务器, 带重试。
 * 成功后缓存时间字符串, 供 OLED / Web 使用。
 *
 * 配置: 修改下面的 NTP_SERVERx / GMT_OFFSET / NTP_RETRY 宏
 * 依赖: WiFi (UDP), time.h
 */

#include "time_manager.h"
#include <Arduino.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

/* ========== 可配置参数 ========== */
#define NTP_SERVER1         "ntp.aliyun.com"
#define NTP_SERVER2         "ntp.tencent.com"
#define NTP_SERVER3         "cn.pool.ntp.org"
#define NTP_GMT_OFFSET_SEC  (8 * 3600)    /* UTC+8 (北京时间) */
#define NTP_DAYLIGHT_SEC    0
#define NTP_RETRY_MAX       30            /* 重试次数 (×100ms = 3s) */
#define NTP_RETRY_MS        100

/* ========== 内部辅助 ========== */

static const char* weekdayName(int wday)
{
    static const char* days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    return (wday >= 0 && wday <= 6) ? days[wday] : "---";
}

/* ========== API ========== */

void Time_Init(TimeManager* tm)
{
    if (!tm) return;
    memset(tm, 0, sizeof(TimeManager));
}

int Time_Sync(TimeManager* tm)
{
    if (!tm) return 0;

    configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_SEC,
               NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

    struct tm timeinfo;
    int retry = 0;
    while (!getLocalTime(&timeinfo) && retry < NTP_RETRY_MAX) {
        delay(NTP_RETRY_MS);
        retry++;
    }

    if (retry >= NTP_RETRY_MAX) return 0;

    strftime(tm->time_str, sizeof(tm->time_str), "%H:%M", &timeinfo);
    strftime(tm->date_str, sizeof(tm->date_str), "%Y-%m-%d", &timeinfo);
    strncpy(tm->weekday_str, weekdayName(timeinfo.tm_wday), sizeof(tm->weekday_str) - 1);
    tm->weekday_str[sizeof(tm->weekday_str) - 1] = '\0';
    tm->synced = 1;
    return 1;
}

void Time_Refresh(TimeManager* tm)
{
    if (!tm || !tm->synced) return;

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return;

    strftime(tm->time_str, sizeof(tm->time_str), "%H:%M", &timeinfo);
    strftime(tm->date_str, sizeof(tm->date_str), "%Y-%m-%d", &timeinfo);
    strncpy(tm->weekday_str, weekdayName(timeinfo.tm_wday), sizeof(tm->weekday_str) - 1);
    tm->weekday_str[sizeof(tm->weekday_str) - 1] = '\0';
}

const char* Time_GetTimeStr(const TimeManager* tm)
{
    return (tm && tm->synced) ? tm->time_str : "--:--";
}

const char* Time_GetDateStr(const TimeManager* tm)
{
    return (tm && tm->synced) ? tm->date_str : "----.--.--";
}

const char* Time_GetWeekdayStr(const TimeManager* tm)
{
    return (tm && tm->synced) ? tm->weekday_str : "---";
}

int Time_IsSynced(const TimeManager* tm)
{
    return tm ? tm->synced : 0;
}
