/**
 * @file    weather_icon.h
 * @brief   OLED 动态天气图标 — GFX 绘图原语实现
 *
 * 支持 OpenWeatherMap 全部 icon 代码, 2 帧动画:
 *   晴天: 太阳光线旋转   少云: 小太阳+云
 *   多云/阴: 云层叠加    阵雨/雨: 云+雨线下落
 *   雷暴: 闪电闪烁+雨    雪: 雪花漂移
 *   雾: 横线波浪         夜间: 弯月+星
 *
 * 16×16 像素区域, 无外部资源依赖。
 *
 * 依赖: Adafruit_GFX (display 全局对象)
 */

#ifndef __WEATHER_ICON_H
#define __WEATHER_ICON_H

/**
 * @brief  在 OLED 上绘制动态天气图标
 * @param  code   OWM icon 代码 (如 "01d", "09d", "11d")
 * @param  x, y   图标区域左上角像素坐标
 * @param  frame  动画帧号 (0 或 1, 由调用方每 500ms 翻转)
 */
void WeatherIcon_Draw(const char* code, int x, int y, int frame);

/**
 * @brief  绘制风向罗盘图标 (16×16)
 * @param  deg   风向角度 (0~360, 0=北=上方)
 * @param  x, y  图标区域左上角像素坐标
 *
 * 绘制圆形罗盘 + 方向箭头, 上北下南。
 * 需要 display 全局对象。
 */
void WeatherIcon_DrawWind(int deg, int x, int y);

/**
 * @brief  绘制湿度水滴图标 (16×16)
 * @param  x, y  图标区域左上角像素坐标
 *
 * 简单水滴造型, 需要 display 全局对象。
 */
void WeatherIcon_DrawHumidity(int x, int y);

#endif /* __WEATHER_ICON_H */
