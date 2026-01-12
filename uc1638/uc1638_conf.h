/*
 * uc1638_conf.h
 * 硬件配置与参数定义
 */

#ifndef __UC1638_CONF_H
#define __UC1638_CONF_H

#include "main.h" // 包含 HAL 库及 GPIO 定义

/* ================= 硬件接口定义 ================= */
// 所有的 Pin 和 Port 宏定义来源于 CubeMX 生成的 main.h
// 请确保在 CubeMX 中将引脚 Label 设为 LCD_CS, LCD_RST, LCD_A0

// 1. SPI 句柄 (根据实际使用的 SPI 修改，如 hspi1, hspi2)
extern SPI_HandleTypeDef hspi1;
#define UC1638_SPI_HANDLE   &hspi1

// 2. 片选信号 (CS) - 低电平有效
#define UC1638_CS_LOW()     HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET)
#define UC1638_CS_HIGH()    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET)

// 3. 复位信号 (RST) - 低电平复位
#define UC1638_RST_LOW()    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET)
#define UC1638_RST_HIGH()   HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET)

// 4. 数据/命令选择 (A0) - 低:命令, 高:数据
#define UC1638_CMD_MODE()   HAL_GPIO_WritePin(LCD_A0_GPIO_Port, LCD_A0_Pin, GPIO_PIN_RESET)
#define UC1638_DATA_MODE()  HAL_GPIO_WritePin(LCD_A0_GPIO_Port, LCD_A0_Pin, GPIO_PIN_SET)

/* ================= 屏幕参数定义 ================= */
#define LCD_WIDTH           128
#define LCD_HEIGHT          128
#define LCD_PAGES           16  // 128 / 8 = 16页
#define LCD_COL_OFFSET      55  // 物理屏幕偏移量 (移植自 Python 驱动)

#endif /* __UC1638_CONF_H */
