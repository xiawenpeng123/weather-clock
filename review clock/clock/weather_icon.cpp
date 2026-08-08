/**
 * @file    weather_icon.cpp
 * @brief   动态天气图标 实现 — GFX 绘图原语 (无浮点运算)
 *
 * 16×16 区域内用圆/线/矩形手绘所有 OWM 天气图标。
 * 2 帧动画 (frame 0/1) 实现旋转、下落、闪烁效果。
 * 太阳射线和风向箭头使用预计算整数查找表, 避免引入 math.h。
 *
 * 依赖: 外部 Adafruit_SSD1306 display 对象 (extern)
 */

#include "weather_icon.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* 需要外部 display 对象 */
extern Adafruit_SSD1306 display;

/* ========== 太阳射线偏移查找表 (16×16 图标内) ========== */
/* 每组 {dx6, dy6, dx8, dy8}: 从圆心 (cx,cy) 到 r=6 和 r=8 的偏移量 */
/* 帧0: 8 条射线 @ 0°/45°/90°/135°/180°/225°/270°/315° */
/* 帧1: 8 条射线 @ 22.5°/67.5°/112.5°/157.5°/202.5°/247.5°/292.5°/337.5° */

static const int8_t PROGMEM sunRayTbl[2][8][4] = {
  /* 帧 0 */
  {{ 0,-6,  0,-8}, { 4,-4,  6,-6}, { 6, 0,  8, 0}, { 4, 4,  6, 6},
   { 0, 6,  0, 8}, {-4, 4, -6, 6}, {-6, 0, -8, 0}, {-4,-4, -6,-6}},
  /* 帧 1 */
  {{ 2,-6,  3,-7}, { 6,-2,  7,-3}, { 6, 2,  7, 3}, { 2, 6,  3, 7},
   {-2, 6, -3, 7}, {-6, 2, -7, 3}, {-6,-2, -7,-3}, {-2,-6, -3,-7}},
};

/* ========== 风向箭头 8 方位查找表 ========== */
/* 方向: N(0), NE(1), E(2), SE(3), S(4), SW(5), W(6), NW(7) */
/* 每组 {tipDx, tipDy, tailDx, tailDy, wing1Dx, wing1Dy, wing2Dx, wing2Dy} */
/* 所有坐标相对于 16×16 图标中心 (cx,cy), dirIdx = ((deg+22)%360)/45 */

static const int8_t PROGMEM windArrowTbl[8][8] = {
  /* N  (0°)   */ { 0,-6,  0, 3, -2,-8,  2,-8},
  /* NE (45°)  */ { 4,-4, -2, 2,  6,-2,  2,-6},
  /* E  (90°)  */ { 6, 0, -3, 0,  4, 2,  4,-2},
  /* SE (135°) */ { 4, 4, -2,-2,  2, 6,  6, 2},
  /* S  (180°) */ { 0, 6,  0,-3, -2, 8,  2, 8},
  /* SW (225°) */ {-4, 4,  2,-2, -6, 2, -2, 6},
  /* W  (270°) */ {-6, 0,  3, 0, -4,-2, -4, 2},
  /* NW (315°) */ {-4,-4,  2, 2, -2,-6, -6,-2},
};

/* ========== 天气图标绘制 ========== */

void WeatherIcon_Draw(const char* code, int x, int y, int frame)
{
    if (!code || code[0] == '\0' || code[1] == '\0') return;

    char c1 = code[0], c2 = code[1];
    int  isNight = (code[2] == 'n');
    int  cx = x + 8, cy = y + 8;  /* 中心点 */

    if (c1 == '0' && c2 == '1') {
        /* ===== 晴天 ===== */
        if (isNight) {
            /* 弯月: 实心圆 + 偏移黑色圆覆盖 */
            display.fillCircle(cx, cy, 6, SSD1306_WHITE);
            display.fillCircle(cx + 2, cy - 1, 4, SSD1306_BLACK);
            /* 星星闪烁 */
            if (frame == 0) {
                display.drawPixel(x + 2, y + 2, SSD1306_WHITE);
                display.drawPixel(x + 14, y + 5, SSD1306_WHITE);
            } else {
                display.drawPixel(x + 3, y + 4, SSD1306_WHITE);
                display.drawPixel(x + 13, y + 12, SSD1306_WHITE);
            }
        } else {
            /* 太阳: 实心圆 + 8条旋转射线 (查找表) */
            display.fillCircle(cx, cy, 5, SSD1306_WHITE);
            for (int i = 0; i < 8; i++) {
                int x1 = cx + (int8_t)pgm_read_byte(&sunRayTbl[frame][i][0]);
                int y1 = cy + (int8_t)pgm_read_byte(&sunRayTbl[frame][i][1]);
                int x2 = cx + (int8_t)pgm_read_byte(&sunRayTbl[frame][i][2]);
                int y2 = cy + (int8_t)pgm_read_byte(&sunRayTbl[frame][i][3]);
                display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
            }
        }

    } else if (c1 == '0' && c2 == '2') {
        /* ===== 少云 ===== */
        display.fillCircle(x + 4, y + 3, 3, SSD1306_WHITE);
        if (frame == 0) {
            display.drawLine(x + 4, y, x + 4, y - 1, SSD1306_WHITE);
            display.drawLine(x + 1, y + 3, x, y + 3, SSD1306_WHITE);
        }
        display.fillCircle(x + 7, y + 10, 3, SSD1306_WHITE);
        display.fillCircle(x + 11, y + 9, 4, SSD1306_WHITE);
        display.fillCircle(x + 14, y + 10, 3, SSD1306_WHITE);
        display.fillRect(x + 7, y + 8, 8, 5, SSD1306_WHITE);

    } else if (c1 == '0' && (c2 == '3' || c2 == '4')) {
        /* ===== 多云 / 阴 ===== */
        display.fillCircle(x + 4, y + 9, 4, SSD1306_WHITE);
        display.fillCircle(x + 8, y + 7, 5, SSD1306_WHITE);
        display.fillCircle(x + 13, y + 9, 4, SSD1306_WHITE);
        display.fillRect(x + 4, y + 8, 10, 6, SSD1306_WHITE);
        if (c2 == '4') {
            /* 阴天: 更厚的云层 */
            display.fillCircle(x + 3, y + 11, 3, SSD1306_WHITE);
            display.fillCircle(x + 14, y + 11, 3, SSD1306_WHITE);
            display.fillRect(x + 3, y + 10, 12, 5, SSD1306_WHITE);
        }

    } else if (c1 == '0' && c2 == '9') {
        /* ===== 阵雨 ===== */
        display.fillCircle(x + 3, y + 3, 3, SSD1306_WHITE);
        display.fillCircle(x + 7, y + 1, 4, SSD1306_WHITE);
        display.fillCircle(x + 11, y + 3, 3, SSD1306_WHITE);
        display.fillRect(x + 3, y + 2, 9, 5, SSD1306_WHITE);
        for (int i = 0; i < 3; i++) {
            int rx = x + 4 + i * 5;
            int ry = y + 9 + ((i + frame) % 3) * 2;
            display.drawLine(rx, ry, rx, ry + 2, SSD1306_WHITE);
        }

    } else if (c1 == '1' && c2 == '0') {
        /* ===== 雨 ===== */
        display.fillCircle(x + 3, y + 3, 3, SSD1306_WHITE);
        display.fillCircle(x + 7, y + 1, 4, SSD1306_WHITE);
        display.fillCircle(x + 11, y + 3, 3, SSD1306_WHITE);
        display.fillRect(x + 3, y + 2, 9, 5, SSD1306_WHITE);
        for (int i = 0; i < 4; i++) {
            int rx = x + 2 + i * 4;
            int ry = y + 9 + ((i + frame * 2) % 4) * 2;
            display.drawLine(rx, ry, rx, ry + 3, SSD1306_WHITE);
        }

    } else if (c1 == '1' && c2 == '1') {
        /* ===== 雷暴 ===== */
        display.fillCircle(x + 3, y + 2, 3, SSD1306_WHITE);
        display.fillCircle(x + 7, y, 4, SSD1306_WHITE);
        display.fillCircle(x + 11, y + 2, 3, SSD1306_WHITE);
        display.fillRect(x + 3, y + 1, 9, 5, SSD1306_WHITE);
        /* 闪电: 仅 frame=0 显示 */
        if (frame == 0) {
            display.drawLine(x + 8, y + 5, x + 5, y + 10, SSD1306_WHITE);
            display.drawLine(x + 5, y + 10, x + 9, y + 10, SSD1306_WHITE);
            display.drawLine(x + 9, y + 10, x + 7, y + 14, SSD1306_WHITE);
        }
        for (int i = 0; i < 2; i++) {
            int rx = x + 3 + i * 8;
            int ry = y + 12 + ((i + frame) % 2) * 2;
            display.drawLine(rx, ry, rx, ry + 2, SSD1306_WHITE);
        }

    } else if (c1 == '1' && c2 == '3') {
        /* ===== 雪 ===== */
        display.fillCircle(x + 3, y + 3, 3, SSD1306_WHITE);
        display.fillCircle(x + 7, y + 1, 4, SSD1306_WHITE);
        display.fillCircle(x + 11, y + 3, 3, SSD1306_WHITE);
        display.fillRect(x + 3, y + 2, 9, 5, SSD1306_WHITE);
        for (int i = 0; i < 4; i++) {
            int sx = x + 2 + i * 4;
            int sy = y + 9 + ((i + frame * 2) % 5);
            /* 雪花: 十字星 */
            display.drawPixel(sx, sy, SSD1306_WHITE);
            display.drawPixel(sx - 1, sy, SSD1306_WHITE);
            display.drawPixel(sx + 1, sy, SSD1306_WHITE);
            display.drawPixel(sx, sy - 1, SSD1306_WHITE);
            display.drawPixel(sx, sy + 1, SSD1306_WHITE);
        }

    } else if (c1 == '5' && c2 == '0') {
        /* ===== 雾 ===== */
        for (int i = 0; i < 4; i++) {
            int ly = y + 3 + i * 4 + (frame % 2);
            int len = (i % 2 == 0) ? 14 : 10;
            display.drawFastHLine(x + 2, ly, len, SSD1306_WHITE);
        }
    }
}

/* ========== 风向罗盘图标 (16×16, 8 方位查找表) ========== */

void WeatherIcon_DrawWind(int deg, int x, int y)
{
    int cx = x + 8, cy = y + 8;

    /* 外圈 */
    display.drawCircle(cx, cy, 7, SSD1306_WHITE);

    /* 十字方位标记: N=上, S=下, E=右, W=左 */
    display.drawPixel(cx,     cy - 7, SSD1306_WHITE);  /* N */
    display.drawPixel(cx,     cy + 7, SSD1306_WHITE);  /* S */
    display.drawPixel(cx + 7, cy,     SSD1306_WHITE);  /* E */
    display.drawPixel(cx - 7, cy,     SSD1306_WHITE);  /* W */

    /* 量化到 8 方位并查表 */
    int dir = ((deg + 22) % 360) / 45;  /* 0=N, 1=NE, ... 7=NW */

    int tipX  = cx + (int8_t)pgm_read_byte(&windArrowTbl[dir][0]);
    int tipY  = cy + (int8_t)pgm_read_byte(&windArrowTbl[dir][1]);
    int tailX = cx + (int8_t)pgm_read_byte(&windArrowTbl[dir][2]);
    int tailY = cy + (int8_t)pgm_read_byte(&windArrowTbl[dir][3]);
    int w1x   = cx + (int8_t)pgm_read_byte(&windArrowTbl[dir][4]);
    int w1y   = cy + (int8_t)pgm_read_byte(&windArrowTbl[dir][5]);
    int w2x   = cx + (int8_t)pgm_read_byte(&windArrowTbl[dir][6]);
    int w2y   = cy + (int8_t)pgm_read_byte(&windArrowTbl[dir][7]);

    /* 箭头主线 */
    display.drawLine(tailX, tailY, tipX, tipY, SSD1306_WHITE);

    /* 箭头翼 (两条线从尖端向后) */
    display.drawLine(tipX, tipY, w1x, w1y, SSD1306_WHITE);
    display.drawLine(tipX, tipY, w2x, w2y, SSD1306_WHITE);

    /* 中心圆点 */
    display.fillCircle(cx, cy, 1, SSD1306_WHITE);
}

/* ========== 湿度水滴图标 (16×16) ========== */

void WeatherIcon_DrawHumidity(int x, int y)
{
    int cx = x + 8, cy = y + 8;

    /* 水滴主体: 上尖下圆的泪滴造型 */
    display.fillCircle(cx, cy + 2, 4, SSD1306_WHITE);
    display.fillTriangle(cx - 4, cy, cx + 4, cy, cx, cy - 5, SSD1306_WHITE);

    /* 高光点 */
    display.fillCircle(cx - 1, cy + 1, 1, SSD1306_BLACK);
}
