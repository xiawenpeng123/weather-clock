/**
 * @file    chinese_font.h
 * @brief   中文字库 — 16×16 点阵存储与绘制
 *
 * 特性:
 *   - 1225 个常用汉字 (全中国县级行政区划地名所含字符), 每字 32 字节 (PROGMEM)
 *   - Unicode 索引 + 二分查找 (O(log N))
 *   - 混合中英文字符串绘制
 *   - 字模由 generate_font.py 从系统宋体生成
 *
 * 依赖: Adafruit_GFX (drawBitmap)
 */

#ifndef __CHINESE_FONT_H
#define __CHINESE_FONT_H

#include <stdint.h>

/* ========== 字库索引条目 ========== */
typedef struct {
    uint16_t code;          /* Unicode 码点 */
    uint16_t glyphIdx;      /* chineseGlyphData[] 中的索引 (×32 字节偏移) */
} ChineseGlyphIdx;

/* ========== API ========== */

/**
 * @brief  二分查找 Unicode 字符在字库中的索引
 * @param  unicode  Unicode 码点 (如 0x5C04 = "射")
 * @return glyph 索引 (0~N-1), 未找到返回 -1
 */
int ChineseFont_FindGlyph(uint16_t unicode);

/**
 * @brief  在 OLED 上绘制一个 16×16 汉字
 * @param  unicode  Unicode 码点
 * @param  x, y     左上角像素坐标
 *
 * 若字符不在字库中, 绘制方框作为占位符。
 * 需要 display 全局对象 (Adafruit_SSD1306)。
 */
void ChineseFont_DrawChar(uint16_t unicode, int x, int y);

/**
 * @brief  解码 UTF-8 字节序列为 Unicode 码点
 * @param  s    指向 UTF-8 字节序列的指针
 * @param  len  输出: 该字符占用的字节数 (1/2/3)
 * @return Unicode 码点, 解码失败返回 0
 */
uint16_t ChineseFont_Utf8ToUnicode(const char* s, int* len);

/**
 * @brief  绘制混合中英文的字符串 (中文 16×16, ASCII 6×8)
 * @param  text  字符串 (UTF-8 编码)
 * @param  x, y  起始像素坐标
 * @return 绘制后的 x 坐标 (可继续拼接)
 *
 * 中文 y 坐标自动上移 4px 使视觉中心对齐 ASCII。
 * 需要 display 全局对象。
 */
int ChineseFont_DrawString(const char* text, int x, int y);

/**
 * @brief  获取混合字符串的像素宽度
 * @param  text  字符串 (UTF-8 编码)
 * @return 像素宽度 (中文=16px, ASCII=6px)
 */
int ChineseFont_GetStringWidth(const char* text);

/**
 * @brief  获取字库中已收录的字符数
 */
int ChineseFont_Count(void);

#endif /* __CHINESE_FONT_H */
