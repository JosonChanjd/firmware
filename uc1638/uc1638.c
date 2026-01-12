/*
 * uc1638.c
 * UC1638 LCD 驱动实现
 * 包含：SPI底层通信、显存管理、图形算法
 */

#include "uc1638.h"
#include "uc1638_font.h"
#include <string.h> // memset
#include <stdlib.h> // abs

// 显存缓冲区：128列 * 16页 = 2048 Bytes
static uint8_t s_DisplayBuf[LCD_PAGES * LCD_WIDTH];

/* ================= 底层 SPI 通信 ================= */

static void UC1638_Write(uint8_t data, uint8_t is_cmd) {
    if (is_cmd) {
        UC1638_CMD_MODE();
    } else {
        UC1638_DATA_MODE();
    }

    UC1638_CS_LOW();
    HAL_SPI_Transmit(UC1638_SPI_HANDLE, &data, 1, 10);
    UC1638_CS_HIGH();
}

// 便捷宏
#define WRITE_CMD(c)  UC1638_Write(c, 1)
#define WRITE_DATA(d) UC1638_Write(d, 0)

/* ================= 初始化与核心控制 ================= */

void UC1638_Init(void) {
    // 1. 硬件复位
    UC1638_RST_LOW();
    HAL_Delay(20);
    UC1638_RST_HIGH();
    HAL_Delay(50); // 等待芯片启动

    // 2. 初始化序列 (完全复刻 Python 代码)
    WRITE_CMD(0xE2); HAL_Delay(5); // System Reset
    
    // --- 显示控制 ---
    WRITE_CMD(0xA4); // Set All Pixel ON -> OFF
    WRITE_CMD(0xA6); // Set Inverse Display -> OFF (正常显示)

    // --- 电源管理 ---
    WRITE_CMD(0xB8); WRITE_DATA(0x00); // LCD Control (MTP)
    WRITE_CMD(0x2D); // Power Control: Enable Internal Charge Pump
    WRITE_CMD(0x20); // Temp Comp
    WRITE_CMD(0xEA); // Bias Setting

    // --- 对比度设置 ---
    WRITE_CMD(0x81); WRITE_DATA(170); // 设置 Vbias (对比度 170)

    // --- 扫描控制 ---
    WRITE_CMD(0xA3); // Set Line Rate
    WRITE_CMD(0xC8); WRITE_DATA(0x2F); // Set COM Scan Direction

    // --- 地址映射 ---
    WRITE_CMD(0x89); WRITE_CMD(0x95); // RAM Address Control
    WRITE_CMD(0x84); // Set COM0
    WRITE_CMD(0xF1); WRITE_DATA(127); // Set COM End
    WRITE_CMD(0xC4); // LCD Map Control
    WRITE_CMD(0x86); // COM Scan Function

    // --- 滚动与窗口 ---
    WRITE_CMD(0x40); WRITE_CMD(0x50); // Set Scroll Line 0
    // [关键] 设置列地址物理偏移 55
    WRITE_CMD(0x04); WRITE_DATA(LCD_COL_OFFSET);

    // 窗口程序 (Window Program)
    WRITE_CMD(0xF4); WRITE_DATA(55);  // Window Start Col
    WRITE_CMD(0xF6); WRITE_DATA(182); // Window End Col
    WRITE_CMD(0xF5); WRITE_DATA(0);   // Window Start Page
    WRITE_CMD(0xF7); WRITE_DATA(15);  // Window End Page
    WRITE_CMD(0xF9); // Window Enable

    // 开启显示
    WRITE_CMD(0xC9); WRITE_DATA(0xAD); // Display Enable
    
    // 清空屏幕
    UC1638_Clear(COLOR_WHITE);
    UC1638_Flush();
}

void UC1638_Clear(LCD_Color_t color) {
    uint8_t val = (color == COLOR_BLACK) ? 0xFF : 0x00;
    memset(s_DisplayBuf, val, sizeof(s_DisplayBuf));
}

void UC1638_Flush(void) {
    uint8_t *pBuf = s_DisplayBuf;

    for (uint8_t page = 0; page < LCD_PAGES; page++) {
        // 1. 设置页地址 (Page Address Set: 0x60 + LSB, 0x70 + MSB)
        WRITE_CMD(0x60 | (page & 0x0F));
        WRITE_CMD(0x70 | (page >> 4));

        // 2. 重置列地址 (每次写入后需要加物理 Offset)
        WRITE_CMD(0x04); 
        WRITE_DATA(LCD_COL_OFFSET);

        // 3. 写入数据指令
        WRITE_CMD(0x01);

        // 4. 批量发送一页数据 (128 Bytes)
        UC1638_DATA_MODE();
        UC1638_CS_LOW();
        HAL_SPI_Transmit(UC1638_SPI_HANDLE, pBuf, LCD_WIDTH, 100);
        UC1638_CS_HIGH();

        pBuf += LCD_WIDTH; // 移动缓冲区指针
    }
}

/* ================= 绘图算法 (移植自 Python) ================= */

void UC1638_DrawPoint(int x, int y, LCD_Color_t color) {
    if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT) return;

    int page = y / 8;
    int bit = y % 8;
    int idx = page * LCD_WIDTH + x;

    if (color == COLOR_BLACK) {
        s_DisplayBuf[idx] |= (1 << bit);
    } else {
        s_DisplayBuf[idx] &= ~(1 << bit);
    }
}

void UC1638_DrawLine(int x1, int y1, int x2, int y2, LCD_Color_t color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        UC1638_DrawPoint(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void UC1638_DrawRectangle(int x1, int y1, int x2, int y2, LCD_Color_t color) {
    UC1638_DrawLine(x1, y1, x2, y1, color);
    UC1638_DrawLine(x1, y1, x1, y2, color);
    UC1638_DrawLine(x1, y2, x2, y2, color);
    UC1638_DrawLine(x2, y1, x2, y2, color);
}

void UC1638_DrawCircle(int x0, int y0, int r, LCD_Color_t color) {
    int a = 0, b = r;
    int d = 3 - (2 * r);
    
    // Bresenham Circle Algorithm
    while (a <= b) {
        UC1638_DrawPoint(x0 - b, y0 - a, color);
        UC1638_DrawPoint(x0 + b, y0 - a, color);
        UC1638_DrawPoint(x0 - a, y0 + b, color);
        UC1638_DrawPoint(x0 - a, y0 - b, color);
        UC1638_DrawPoint(x0 + b, y0 + a, color);
        UC1638_DrawPoint(x0 + a, y0 - b, color);
        UC1638_DrawPoint(x0 + a, y0 + b, color);
        UC1638_DrawPoint(x0 - b, y0 + a, color);
        
        a++;
        if (d < 0) {
            d += 4 * a + 6;
        } else {
            d += 4 * (a - b) + 10;
            b--;
        }
    }
}

// 优化的区域填充算法
void UC1638_Fill(int x1, int y1, int x2, int y2, LCD_Color_t color) {
    if (x1 >= LCD_WIDTH) x1 = LCD_WIDTH - 1;
    if (x2 >= LCD_WIDTH) x2 = LCD_WIDTH - 1;
    if (y1 >= LCD_HEIGHT) y1 = LCD_HEIGHT - 1;
    if (y2 >= LCD_HEIGHT) y2 = LCD_HEIGHT - 1;

    int page_start = y1 / 8;
    int page_end = y2 / 8;
    int row1 = y1 % 8;
    int row2 = y2 % 8;

    for (int page = page_start; page <= page_end; page++) {
        int r_s = (page == page_start) ? row1 : 0;
        int r_e = (page == page_end) ? row2 : 7;
        
        uint8_t mask = 0;
        for (int i = r_s; i <= r_e; i++) {
            mask |= (1 << i);
        }

        for (int col = x1; col <= x2; col++) {
            int idx = page * LCD_WIDTH + col;
            if (color == COLOR_BLACK) {
                s_DisplayBuf[idx] |= mask;
            } else {
                s_DisplayBuf[idx] &= ~mask;
            }
        }
    }
}

/* ================= 文本显示 ================= */

void UC1638_ShowChar(int x, int y, char chr, LCD_Color_t color) {
    const uint8_t *pFont = Get_Font_Pointer(chr);
    
    // 6x12 Font, 逐列绘制
    for (int h = 0; h < 12; h++) {
        for (int w = 0; w < 6; w++) {
            // 检查字模中的位是否为1
            if (pFont[h] & (1 << w)) {
                UC1638_DrawPoint(x + w, y + h, color);
            }
        }
    }
}

void UC1638_ShowString(int x, int y, const char *str, LCD_Color_t color) {
    while (*str) {
        UC1638_ShowChar(x, y, *str, color);
        x += 6; // 字符宽度
        str++;
    }
}

void UC1638_ShowInt(int x, int y, int num, int len, LCD_Color_t color) {
    char buf[12];
    // 简易 int 转 string，不支持负数补码，仅做演示
    if(len > 10) len = 10;
    
    int t;
    int enshow = 0;
    
    for (int i = 0; i < len; i++) {
        // 计算当前位的除数 (例如 len=5, i=0, div=10000)
        int div = 1;
        for(int k=0; k < len-1-i; k++) div *= 10;
        
        t = (num / div) % 10;
        
        if (enshow == 0 && i < (len - 1)) {
            if (t == 0) {
                UC1638_ShowChar(x + i*6, y, ' ', color);
                continue;
            } else {
                enshow = 1;
            }
        }
        UC1638_ShowChar(x + i*6, y, t + '0', color);
    }
}

/* ================= 演示图案 (Demo Logic) ================= */

void UC1638_Demo_Checkerboard(void) {
    UC1638_Clear(COLOR_WHITE);
    int cols_range[3][2] = {{0, 42}, {43, 85}, {86, 127}};
    int rows_range[3][2] = {{0, 42}, {43, 85}, {86, 127}};

    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            if ((r + c) % 2 == 0) {
                UC1638_Fill(cols_range[c][0], rows_range[r][0],
                            cols_range[c][1], rows_range[r][1], 
                            COLOR_BLACK);
            }
        }
    }
}

void UC1638_Demo_SplitScreen(void) {
    UC1638_Clear(COLOR_WHITE);
    UC1638_Fill(0, 64, 127, 127, COLOR_BLACK);
}
