/**
 * @file    display_manager.cpp
 * @brief   OLED 显示管理器 实现
 *
 * 画面布局 (128×64):
 *   y=0:  星期 + 日期 (ASCII 6×8, size 1)
 *   y=9:   HH:MM + 天气描述 (时间大字 size 2 + 中文天气, 最多2字)
 *   y=27: 分隔横线
 *   y=30: 4 个图标 (16×16) — 天气 / 风向 / 湿度 / 温度
 *   y=48: 4 个数据标签 — 温度°C / 风速 / 湿度% / 城市 (数值 y=48, 单位 y=56/58, 城市 y=47)
 *
 * 全部 4 列均匀分布在 128px 上, 列中心: 16 / 48 / 80 / 112
 *
 * 依赖: Adafruit_SSD1306, chinese_font, weather_icon, time_manager, weather_client
 * 配置: 修改下面的 DISP_* / ANIM_FRAME_MS 宏
 */

#include "display_manager.h"
#include "time_manager.h"
#include "weather_client.h"
#include "chinese_font.h"
#include "weather_icon.h"
#include <Adafruit_SSD1306.h>
#include <stdio.h>
#include <string.h>

/* 需要外部 display 对象 (全局, clock.ino 中定义) */
extern Adafruit_SSD1306 display;

/* ========== 可配置参数 ========== */
#define DISP_SCREEN_WIDTH   128
#define DISP_SCREEN_HEIGHT  64
#define DISP_ANIM_FRAME_MS  500    /* 动画帧切换间隔 */

/* ========== 内部辅助 ========== */

static int centerX(int textLen, int charWidth)
{
    int x = (DISP_SCREEN_WIDTH - textLen * charWidth) / 2;
    return x < 0 ? 0 : x;
}

/* ========== API ========== */

void Display_Init(DisplayManager* dm, Adafruit_SSD1306* oled, const char* city_name)
{
    if (!dm) return;
    dm->oled          = oled;
    dm->anim_frame    = 0;
    dm->last_anim_ms  = 0;
    if (city_name) {
        strncpy(dm->city_name, city_name, DISP_CITY_NAME_MAX - 1);
        dm->city_name[DISP_CITY_NAME_MAX - 1] = '\0';
    } else {
        dm->city_name[0] = '\0';
    }
}

void Display_SetCityName(DisplayManager* dm, const char* city_name)
{
    if (!dm || !city_name) return;
    strncpy(dm->city_name, city_name, DISP_CITY_NAME_MAX - 1);
    dm->city_name[DISP_CITY_NAME_MAX - 1] = '\0';
}

void Display_ShowBootMsg(DisplayManager* dm, const char* msg)
{
    if (!dm || !dm->oled) return;
    dm->oled->clearDisplay();
    dm->oled->setTextColor(SSD1306_WHITE);
    dm->oled->setTextSize(1);
    int len = strlen(msg);
    dm->oled->setCursor(centerX(len, 6), 24);
    dm->oled->print(msg);
    dm->oled->display();
}

void Display_ShowMultiMsg(DisplayManager* dm,
                          const char* line1, const char* line2, const char* line3)
{
    if (!dm || !dm->oled) return;
    dm->oled->clearDisplay();
    dm->oled->setTextColor(SSD1306_WHITE);
    dm->oled->setTextSize(1);

    if (line1) {
        int len = strlen(line1);
        dm->oled->setCursor(centerX(len, 6), 8);
        dm->oled->print(line1);
    }
    if (line2) {
        int len = strlen(line2);
        dm->oled->setCursor(centerX(len, 6), 24);
        dm->oled->print(line2);
    }
    if (line3) {
        int len = strlen(line3);
        dm->oled->setCursor(centerX(len, 6), 40);
        dm->oled->print(line3);
    }
    dm->oled->display();
}

/* 4 列均匀分布: 列中心 x 坐标 */
static const int COL_CX[4] = { 16, 48, 80, 112 };

/* 辅助: 在指定列中心绘制 text (size 1), y 为文字左上角 */
static void drawCenteredText(const char* text, int colIdx, int y)
{
    if (!text || text[0] == '\0') return;
    int w = strlen(text) * 6;  /* size 1: 6px/char */
    int x = COL_CX[colIdx] - w / 2;
    if (x < 0) x = 0;
    display.setCursor(x, y);
    display.print(text);
}

/* 辅助: 在指定列中心绘制中文 (每字 16px 宽) */
static void drawCenteredChinese(const char* text, int colIdx, int y)
{
    if (!text || text[0] == '\0') return;
    int w = ChineseFont_GetStringWidth(text);
    int x = COL_CX[colIdx] - w / 2;
    if (x < 0) x = 0;
    ChineseFont_DrawString(text, x, y);
}

/**
 * @brief  截断 UTF-8 字符串, 最多保留 maxChars 个可渲染的中文字符
 *
 * 只保留字库中存在的 CJK 字符, 跳过 ASCII/空格/不可渲染字符。
 * 原地修改, 在截断位置写入 '\0'。
 */
static void truncateWeatherToChars(char* str, int maxChars)
{
    if (!str || maxChars <= 0) return;
    const char* src = str;
    char* dst = str;
    int count = 0;

    while (*src && count < maxChars) {
        int len;
        uint16_t uc = ChineseFont_Utf8ToUnicode(src, &len);
        /* 只保留多字节 CJK 字符 (跳过 ASCII / 空格 / 标点等) */
        if (len >= 2 && ChineseFont_FindGlyph(uc) >= 0) {
            /* 字库中存在, 复制到 dst */
            for (int i = 0; i < len; i++) {
                dst[i] = src[i];
            }
            dst += len;
            count++;
        }
        src += len;
    }
    *dst = '\0';
}

void Display_Update(DisplayManager* dm, const TimeManager* tm, const WeatherClient* wc)
{
    if (!dm || !dm->oled) return;

    Adafruit_SSD1306& d = *dm->oled;

    /* 动画帧切换 */
    unsigned long now = millis();
    if (now - dm->last_anim_ms > DISP_ANIM_FRAME_MS) {
        dm->anim_frame = (dm->anim_frame + 1) % 2;
        dm->last_anim_ms = now;
    }

    d.clearDisplay();
    d.setTextColor(SSD1306_WHITE);

    /* ========== 第 1 行: 星期 + 日期 ========== */
    /* 用 char 数组代替 String 拼接 */
    char line1[32];
    snprintf(line1, sizeof(line1), "%s  %s",
             Time_GetWeekdayStr(tm), Time_GetDateStr(tm));
    d.setTextSize(1);
    d.setCursor(centerX(strlen(line1), 6), 0);
    d.print(line1);

    /* ========== 第 2 行: 时间 (大字) + 天气描述 (最多2字) ========== */
    const char* timeStr = Time_GetTimeStr(tm);
    char weatherDesc[16];
    strncpy(weatherDesc, Weather_GetDesc(wc), sizeof(weatherDesc) - 1);
    weatherDesc[sizeof(weatherDesc) - 1] = '\0';
    truncateWeatherToChars(weatherDesc, 2);  /* 天气描述限制为2个可渲染汉字 */

    int timeW = strlen(timeStr) * 12;         /* size 2: 12px/char */
    int descW = ChineseFont_GetStringWidth(weatherDesc);
    int gap = 9;                               /* 时间与描述间距 */
    int groupW = timeW + gap + descW;
    int groupX = (DISP_SCREEN_WIDTH - groupW) / 2;
    if (groupX < 0) groupX = 0;

    d.setTextSize(2);
    d.setCursor(groupX, 9);
    d.print(timeStr);

    /* 天气描述紧随时间右侧, y=13 使中文与 size 2 文本视觉对齐 */
    ChineseFont_DrawString(weatherDesc, groupX + timeW + gap, 13);

    /* ========== 分隔线 ========== */
    d.drawFastHLine(8, 27, 112, SSD1306_WHITE);

    /* ========== 第 3 行: 4 个图标 (16×16), y=30 ========== */
    /* 列 1: 天气图标 */
    WeatherIcon_Draw(Weather_GetIcon(wc), COL_CX[0] - 8, 30, dm->anim_frame);

    /* 列 2: 风向罗盘 */
    WeatherIcon_DrawWind(Weather_GetWindDeg(wc), COL_CX[1] - 8, 30);

    /* 列 3: 湿度水滴 */
    WeatherIcon_DrawHumidity(COL_CX[2] - 8, 30);

    /* 列 4: 温度图标 (简易温度计) */
    {
        int tx = COL_CX[3], ty = 30;
        d.drawCircle(tx, ty + 11, 3, SSD1306_WHITE);
        d.fillCircle(tx, ty + 11, 1, SSD1306_WHITE);
        d.drawFastVLine(tx, ty + 2, 7, SSD1306_WHITE);
        d.drawFastHLine(tx - 1, ty + 3, 3, SSD1306_WHITE);
    }

    /* ========== 第 4 行: 数据标签, 上下两行: 数值 (y=48) + 单位 (y=56) ========== */
    d.setTextSize(1);

    /* 列 1: 温度 — 数值在上, °C 在下 */
    {
        const char* tempStr = Weather_GetTemp(wc);
        int tw = strlen(tempStr) * 6;
        int tx = COL_CX[0] - tw / 2;
        if (tx < 0) tx = 0;
        d.setCursor(tx, 48);
        d.print(tempStr);

        /* 手绘 °C: 圆圈 + C, 居中于列 1 */
        int degW = 10;  /* 圆圈(4px) + C(6px) */
        int ux = COL_CX[0] - degW / 2;
        if (ux < 0) ux = 0;
        d.fillCircle(ux + 3, 59, 2, SSD1306_WHITE);
        d.setCursor(ux + 6, 58);
        d.print("C");
    }

    /* 列 2: 风速 — 数值在上, m/s 在下 */
    {
        const char* wsStr = Weather_GetWindSpeed(wc);
        int vw = strlen(wsStr) * 6;
        int vx = COL_CX[1] - vw / 2;
        if (vx < 0) vx = 0;
        d.setCursor(vx, 48);
        d.print(wsStr);

        int uw = 3 * 6;  /* "m/s" */
        int ux = COL_CX[1] - uw / 2;
        if (ux < 0) ux = 0;
        d.setCursor(ux, 56);
        d.print("m/s");
    }

    /* 列 3: 湿度 — 数值在上, % 在下 */
    {
        const char* humStr = Weather_GetHumidity(wc);
        int vw = strlen(humStr) * 6;
        int vx = COL_CX[2] - vw / 2;
        if (vx < 0) vx = 0;
        d.setCursor(vx, 48);
        d.print(humStr);

        int uw = 1 * 6;  /* "%" */
        int ux = COL_CX[2] - uw / 2;
        if (ux < 0) ux = 0;
        d.setCursor(ux, 56);
        d.print("%");
    }

    /* 列 4: 城市名 — 逐字绘制, "阳"(0x9633) 下移 2px */
    {
        const char* city = dm->city_name;
        if (city && city[0] != '\0') {
            int cityW = ChineseFont_GetStringWidth(city);
            int cx = COL_CX[3] - cityW / 2;
            if (cx < 0) cx = 0;
            int len;
            while (*city) {
                uint16_t uc = ChineseFont_Utf8ToUnicode(city, &len);
                if (len > 1) {
                    int cy = (uc == 0x9633) ? 49 : 47;  /* 阳下移 2px */
                    ChineseFont_DrawChar(uc, cx, cy);
                    cx += 16;
                }
                city += len;
            }
        }
    }

    d.display();
}
