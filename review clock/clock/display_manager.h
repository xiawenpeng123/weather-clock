/**
 * @file    display_manager.h
 * @brief   OLED 显示管理器 — 画面布局与帧刷新
 *
 * 管理整个 128×64 画面的渲染:
 *   第1行: 星期 + 日期
 *   第2行: HH:MM (大字)
 *   分隔线
 *   第3行: 4 图标 (天气/风向/湿度/温度) — 均匀分布
 *   第4行: 4 标签 (天气描述/风速/湿度%/城市) — 均匀分布
 *   第5行: 温度值 + °C (居中)
 *
 * 每 500ms 自动翻转动画帧。
 *
 * 依赖: Adafruit_SSD1306, chinese_font, weather_icon
 */

#ifndef __DISPLAY_MANAGER_H
#define __DISPLAY_MANAGER_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdint.h>

/* 前向声明 */
struct TimeManager;
struct WeatherClient;

#define DISP_CITY_NAME_MAX  32

/* ========== 状态 ========== */
typedef struct {
    Adafruit_SSD1306* oled;          /* OLED 实例指针 */
    uint8_t           anim_frame;     /* 动画帧 (0/1) */
    unsigned long     last_anim_ms;  /* 上次动画帧切换时间 */
    char              city_name[DISP_CITY_NAME_MAX]; /* 城市名 (UTF-8) */
} DisplayManager;

/* ========== API ========== */

/**
 * @brief  初始化显示管理器
 * @param  dm        管理器实例
 * @param  oled      已初始化的 SSD1306 对象指针
 * @param  city_name 城市名 (如 "射阳")
 */
void Display_Init(DisplayManager* dm, Adafruit_SSD1306* oled, const char* city_name);

/**
 * @brief  运行时更新城市名 (用于网页切换城市后同步 OLED)
 * @param  dm        管理器实例
 * @param  city_name 新城市名 (UTF-8)
 */
void Display_SetCityName(DisplayManager* dm, const char* city_name);

/**
 * @brief  在 OLED 中央显示单行启动提示文字
 * @param  dm   管理器实例
 * @param  msg  提示文字 (ASCII, 建议 ≤14 字符)
 */
void Display_ShowBootMsg(DisplayManager* dm, const char* msg);

/**
 * @brief  在 OLED 中央显示三行提示文字 (用于 WiFi 配置门户)
 * @param  dm     管理器实例
 * @param  line1  第 1 行 (y=8, 标题)
 * @param  line2  第 2 行 (y=24, SSID)
 * @param  line3  第 3 行 (y=40, IP 地址)
 */
void Display_ShowMultiMsg(DisplayManager* dm,
                          const char* line1, const char* line2, const char* line3);

/**
 * @brief  完整刷新一帧 OLED 画面
 * @param  dm      管理器实例
 * @param  tm      时间数据 (只读)
 * @param  wc      天气数据 (只读)
 *
 * 内部自动处理动画帧切换 (每 ANIM_FRAME_MS)。
 * 如果时间/天气尚未就绪, 显示 "--:--" / "loading"。
 */
void Display_Update(DisplayManager* dm, const TimeManager* tm, const WeatherClient* wc);

#endif /* __DISPLAY_MANAGER_H */
