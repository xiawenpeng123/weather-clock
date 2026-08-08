/**
 * @file    time_manager.h
 * @brief   NTP 时间同步与管理
 *
 * 配置 3 个 NTP 服务器, 带重试。
 * 时间字符串缓存到结构体中供 OLED / Web 使用。
 *
 * 依赖: WiFi (UDP), time.h (configTime / getLocalTime)
 */

#ifndef __TIME_MANAGER_H
#define __TIME_MANAGER_H

#include <stdint.h>

/* ========== 状态 ========== */
typedef struct TimeManager {
    char time_str[8];        /* "HH:MM" */
    char date_str[16];       /* "YYYY-MM-DD" */
    char weekday_str[8];     /* "Mon" / "Tue" ... */
    uint8_t synced;          /* 是否成功同步过 */
} TimeManager;

/* ========== API ========== */

/**
 * @brief  初始化时间管理器 (归零所有字段)
 */
void Time_Init(TimeManager* tm);

/**
 * @brief  执行 NTP 同步 (阻塞, 最长 ~3s)
 * @param  tm  时间管理器实例
 * @return 1=成功, 0=超时/失败
 */
int Time_Sync(TimeManager* tm);

/**
 * @brief  刷新本地时间缓存 (不发起网络请求)
 * @param  tm  时间管理器实例
 * @note   从 ESP32 系统时钟 (getLocalTime) 读取当前时间并更新缓存字符串。
 *         调用频率: 每秒一次, 确保 OLED 时间实时更新。
 *         仅在 synced==1 时有效, 否则保持 "--:--" 占位符。
 */
void Time_Refresh(TimeManager* tm);

/* ========== 只读访问器 ========== */
const char* Time_GetTimeStr(const TimeManager* tm);
const char* Time_GetDateStr(const TimeManager* tm);
const char* Time_GetWeekdayStr(const TimeManager* tm);
int         Time_IsSynced(const TimeManager* tm);

#endif /* __TIME_MANAGER_H */
