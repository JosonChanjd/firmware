/*
 * uc1638.h
 * UC1638 LCD 驱动 API 声明
 */

#ifndef __UC1638_H
#define __UC1638_H

#include <stdint.h>
#include "uc1638_conf.h"

// 颜色定义
typedef enum {
    COLOR_WHITE = 0,
    COLOR_BLACK = 1
} LCD_Color_t;

// 核心功能
void UC1638_Init(void);
void UC1638_Flush(void); // 将显存刷新到屏幕
void UC1638_Clear(LCD_Color_t color);

// 绘图 API
void UC1638_DrawPoint(int x, int y, LCD_Color_t color);
void UC1638_DrawLine(int x1, int y1, int x2, int y2, LCD_Color_t color);
void UC1638_DrawRectangle(int x1, int y1, int x2, int y2, LCD_Color_t color);
void UC1638_DrawCircle(int x0, int y0, int r, LCD_Color_t color);
void UC1638_Fill(int x1, int y1, int x2, int y2, LCD_Color_t color); // 区域填充（高性能）

// 文本 API
void UC1638_ShowChar(int x, int y, char chr, LCD_Color_t color);
void UC1638_ShowString(int x, int y, const char *str, LCD_Color_t color);
void UC1638_ShowInt(int x, int y, int num, int len, LCD_Color_t color);

// 演示功能 (对应 Python 逻辑)
void UC1638_Demo_Checkerboard(void);
void UC1638_Demo_SplitScreen(void);

#endif /* __UC1638_H */
