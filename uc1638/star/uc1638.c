/*
P3PLUS LCD (UC1638) ESP-IDF 驱动 - MOSI=36 适配版
适配 ESP32-S3 + VSCode + ESP-IDF v5.1.2
核心配置：SPI2_HOST + SCLK=18 + MOSI=36
已修复所有编译错误：无pow函数、引脚全数字、头文件依赖正常
*/
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"

// ====================== 1. 硬件引脚配置（核心修改：MOSI=36） ======================
#define LCD_SCLK_PIN    18           // SPI2_SCLK（时钟，固定）
#define LCD_MOSI_PIN    36           // SPI2_MOSI（数据输出，适配开发板）
#define LCD_DC_PIN      5            // 命令/数据选择（A0）
#define LCD_CS_PIN      4            // 片选（低有效）
#define LCD_RST_PIN     2            // 硬件复位
#define EXIT_KEY_PIN    0            // 退出按键

// ====================== 2. 宏定义与常量 ======================
#define TAG             "P3PLUS_LCD"
#define SPI_HOST        SPI2_HOST     // 保留SPI2_HOST，匹配36脚的SPI2映射
#define SPI_CLOCK_HZ    2000000       // SPI 时钟 2MHz（兼容UC1638）
#define LCD_COLS        128           // 列数
#define LCD_ROWS        128           // 行数
#define LCD_PAGES       16            // 页数（128行/8位=16页）
#define LCD_BUF_SIZE    (LCD_PAGES * LCD_COLS)

// ====================== 3. 全局变量 ======================
spi_device_handle_t spi_handle;       // SPI 设备句柄
uint8_t display_buffer[LCD_BUF_SIZE]; // 显存缓冲区（128*16=2048字节）

// ====================== 4. 工具函数（整数幂运算，替代pow） ======================
uint32_t power_of_10(uint8_t n) {
    uint32_t result = 1;
    for (uint8_t i = 0; i < n; i++) {
        result *= 10;
    }
    return result;
}

// ====================== 5. LCD 字体数据 ======================
uint8_t* get_ascii_1206_font(uint8_t char_code) {
    static uint8_t default_font[12] = {0x00,0x1E,0x21,0x21,0x21,0x1E,0x00,0x00,0x00,0x00,0x00,0x00};
    static uint8_t ascii_1206[128][12] = {
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 0 (空)
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 32 (空格)
        {0x00,0x00,0x04,0x04,0x04,0x04,0x04,0x00,0x00,0x04,0x00,0x00}, // 33 (!)
        {0x00,0x00,0x0E,0x11,0x11,0x11,0x11,0x11,0x11,0x0E,0x00,0x00}, // 48 (0)
        {0x00,0x00,0x04,0x06,0x04,0x04,0x04,0x04,0x04,0x0E,0x00,0x00}, // 49 (1)
        {0x00,0x00,0x0E,0x11,0x11,0x08,0x04,0x02,0x01,0x1F,0x00,0x00}, // 50 (2)
        {0x00,0x00,0x0E,0x11,0x10,0x0C,0x10,0x10,0x11,0x0E,0x00,0x00}, // 51 (3)
        {0x00,0x00,0x08,0x0C,0x0C,0x0A,0x09,0x1F,0x08,0x1C,0x00,0x00}, // 52 (4)
        {0x00,0x00,0x1F,0x01,0x01,0x0F,0x11,0x10,0x11,0x0E,0x00,0x00}, // 53 (5)
        {0x00,0x00,0x0C,0x12,0x01,0x0D,0x13,0x11,0x11,0x0E,0x00,0x00}, // 54 (6)
        {0x00,0x00,0x1E,0x10,0x08,0x08,0x04,0x04,0x04,0x04,0x00,0x00}, // 55 (7)
        {0x00,0x00,0x0E,0x11,0x11,0x0E,0x11,0x11,0x11,0x0E,0x00,0x00}, // 56 (8)
        {0x00,0x00,0x0E,0x11,0x11,0x19,0x16,0x10,0x09,0x06,0x00,0x00}, // 57 (9)
        {0x00,0x00,0x04,0x04,0x0C,0x0A,0x0A,0x1E,0x12,0x33,0x00,0x00}, // 65 (A)
        {0x00,0x00,0x0F,0x12,0x12,0x0E,0x12,0x12,0x12,0x0F,0x00,0x00}, // 66 (B)
        {0x00,0x00,0x1E,0x11,0x01,0x01,0x01,0x01,0x11,0x0E,0x00,0x00}, // 67 (C)
        {0x00,0x00,0x0F,0x12,0x12,0x12,0x12,0x12,0x12,0x0F,0x00,0x00}, // 68 (D)
        {0x00,0x00,0x1F,0x12,0x0A,0x0E,0x0A,0x02,0x12,0x1F,0x00,0x00}, // 69 (E)
        {0x00,0x00,0x33,0x12,0x12,0x1E,0x12,0x12,0x12,0x33,0x00,0x00}, // 72 (H)
        {0x00,0x00,0x07,0x02,0x02,0x02,0x02,0x02,0x22,0x3F,0x00,0x00}, // 76 (L)
        {0x00,0x00,0x3B,0x1B,0x1B,0x1B,0x15,0x15,0x15,0x35,0x00,0x00}, // 77 (M)
        {0x00,0x00,0x0F,0x12,0x12,0x0E,0x02,0x02,0x02,0x07,0x00,0x00}, // 80 (P)
        {0x00,0x00,0x0E,0x11,0x01,0x0E,0x10,0x11,0x11,0x0E,0x00,0x00}, // 83 (S)
        {0x00,0x00,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x0E,0x00,0x00}, // 85 (U)
        {0x00,0x00,0x00,0x00,0x00,0x0C,0x12,0x1E,0x02,0x1C,0x00,0x00}, // 101 (e)
        {0x00,0x07,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x1F,0x00,0x00}, // 108 (l)
        {0x00,0x00,0x00,0x00,0x00,0x0C,0x12,0x12,0x12,0x0C,0x00,0x00}, // 111 (o)
    };

    if (char_code >= 0 && char_code < 128) {
        return ascii_1206[char_code];
    } else {
        return default_font; // 未知字符返回方块
    }
}

// ====================== 6. SPI 与 LCD 底层操作 ======================
void lcd_spi_send(uint8_t data, bool is_cmd) {
    esp_err_t ret;
    spi_transaction_t t = {0};
    gpio_set_level(LCD_DC_PIN, is_cmd ? 0 : 1);
    t.length = 8;
    t.tx_buffer = &data;
    t.user = NULL;
    ret = spi_device_transmit(spi_handle, &t);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI发送失败: %d", ret);
    }
}

void lcd_spi_send_bulk(uint8_t* data, uint16_t len) {
    esp_err_t ret;
    spi_transaction_t t = {0};
    gpio_set_level(LCD_DC_PIN, 1);
    t.length = len * 8;
    t.tx_buffer = data;
    ret = spi_device_transmit(spi_handle, &t);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI批量发送失败: %d", ret);
    }
}

void lcd_hw_reset(void) {
    gpio_set_level(LCD_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(LCD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(20));
}

void lcd_init(void) {
    lcd_hw_reset();

    // 软复位
    lcd_spi_send(0xE1, true);
    lcd_spi_send(0xE2, true);
    vTaskDelay(pdMS_TO_TICKS(2));

    // 显示控制
    lcd_spi_send(0xA4, true);
    lcd_spi_send(0xA6, true);

    // 电源管理
    lcd_spi_send(0xB8, true); lcd_spi_send(0x00, false);
    lcd_spi_send(0x2D, true);
    lcd_spi_send(0x20, true);
    lcd_spi_send(0xEA, true);

    // 对比度
    lcd_spi_send(0x81, true); lcd_spi_send(170, false);

    // 扫描控制
    lcd_spi_send(0xA3, true);
    lcd_spi_send(0xC8, true); lcd_spi_send(0x2F, false);

    // 地址映射
    lcd_spi_send(0x89, true); lcd_spi_send(0x95, true);
    lcd_spi_send(0x84, true);
    lcd_spi_send(0xF1, true); lcd_spi_send(127, false);
    lcd_spi_send(0xC4, true);
    lcd_spi_send(0x86, true);

    // 滚动与窗口
    lcd_spi_send(0x40, true); lcd_spi_send(0x50, true);
    lcd_spi_send(0x04, true); lcd_spi_send(55, false);

    // 窗口范围
    lcd_spi_send(0xF4, true); lcd_spi_send(55, false);
    lcd_spi_send(0xF6, true); lcd_spi_send(182, false);
    lcd_spi_send(0xF5, true); lcd_spi_send(0, false);
    lcd_spi_send(0xF7, true); lcd_spi_send(15, false);
    lcd_spi_send(0xF9, true);

    lcd_spi_send(0xC9, true); lcd_spi_send(0xAD, false);
    ESP_LOGI(TAG, "LCD初始化完成");
}

void spi_bus_init(void) {
    esp_err_t ret;
    spi_bus_config_t bus_cfg = {
        .miso_io_num = -1,        // LCD仅写，MISO悬空
        .mosi_io_num = LCD_MOSI_PIN, // 36脚（修改后）
        .sclk_io_num = LCD_SCLK_PIN, // 18脚
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_BUF_SIZE,
    };

    ret = spi_bus_initialize(SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = SPI_CLOCK_HZ,
        .mode = 0,
        .spics_io_num = LCD_CS_PIN,
        .queue_size = 7,
        .pre_cb = NULL,
        .post_cb = NULL,
    };

    ret = spi_bus_add_device(SPI_HOST, &dev_cfg, &spi_handle);
    ESP_ERROR_CHECK(ret);

    // 初始化DC/RST引脚
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LCD_DC_PIN) | (1ULL << LCD_RST_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // 初始化退出按键
    gpio_config_t key_conf = {
        .pin_bit_mask = (1ULL << EXIT_KEY_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&key_conf);

    ESP_LOGI(TAG, "SPI总线初始化完成");
}

// ====================== 7. 图形绘制函数 ======================
void lcd_clear_screen(uint8_t color) {
    uint8_t val = color ? 0xFF : 0x00;
    memset(display_buffer, val, LCD_BUF_SIZE);
}

void lcd_draw_point(uint16_t x, uint16_t y, uint8_t color) {
    if (x >= LCD_COLS || y >= LCD_ROWS) return;
    uint8_t page = y >> 3;
    uint8_t row = y & 0x07;
    uint16_t idx = page * LCD_COLS + x;

    if (color) {
        display_buffer[idx] |= (1 << row);
    } else {
        display_buffer[idx] &= ~(1 << row);
    }
}

void lcd_fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color) {
    x1 = x1 > LCD_COLS-1 ? LCD_COLS-1 : x1;
    x2 = x2 > LCD_COLS-1 ? LCD_COLS-1 : x2;
    y1 = y1 > LCD_ROWS-1 ? LCD_ROWS-1 : y1;
    y2 = y2 > LCD_ROWS-1 ? LCD_ROWS-1 : y2;

    color = color & 1;
    uint8_t page1 = y1 >> 3;
    uint8_t page2 = y2 >> 3;
    uint8_t row1 = y1 & 0x07;
    uint8_t row2 = y2 & 0x07;

    for (uint8_t page = page1; page <= page2; page++) {
        uint8_t row_start = (page == page1) ? row1 : 0;
        uint8_t row_end = (page == page2) ? row2 : 7;

        uint8_t mask = 0;
        for (uint8_t i = row_start; i <= row_end; i++) {
            mask |= (1 << i);
        }

        for (uint16_t col = x1; col <= x2; col++) {
            uint16_t idx = page * LCD_COLS + col;
            if (color) {
                display_buffer[idx] |= mask;
            } else {
                display_buffer[idx] &= ~mask;
            }
        }
    }
}

void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color) {
    int16_t dx = abs(x2 - x1);
    int16_t dy = abs(y2 - y1);
    int16_t sx = x1 < x2 ? 1 : -1;
    int16_t sy = y1 < y2 ? 1 : -1;
    int16_t err = dx - dy;

    while (1) {
        lcd_draw_point(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int16_t e2 = 2 * err;
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

void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color) {
    lcd_draw_line(x1, y1, x2, y1, color);
    lcd_draw_line(x1, y1, x1, y2, color);
    lcd_draw_line(x1, y2, x2, y2, color);
    lcd_draw_line(x2, y1, x2, y2, color);
}

void lcd_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint8_t color) {
    int16_t a = 0, b = r;
    while (a <= b) {
        lcd_draw_point(x0 - b, y0 - a, color);
        lcd_draw_point(x0 + b, y0 - a, color);
        lcd_draw_point(x0 - a, y0 + b, color);
        lcd_draw_point(x0 - a, y0 - b, color);
        lcd_draw_point(x0 + b, y0 + a, color);
        lcd_draw_point(x0 + a, y0 - b, color);
        lcd_draw_point(x0 + a, y0 + b, color);
        lcd_draw_point(x0 - b, y0 + a, color);
        a++;
        if ((a*a + b*b) > (r*r)) b--;
    }
}

void lcd_show_char(uint16_t x, uint16_t y, char ch, uint8_t fc, uint8_t bc, uint8_t size) {
    if (size != 12) return;
    uint8_t* font = get_ascii_1206_font((uint8_t)ch);

    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 6; j++) {
            if (font[i] & (0x01 << j)) {
                lcd_draw_point(x + j, y + i, fc);
            } else {
                lcd_draw_point(x + j, y + i, bc);
            }
        }
    }
}

void lcd_show_string(uint16_t x, uint16_t y, const char* str, uint8_t fc, uint8_t bc, uint8_t size) {
    while (*str) {
        lcd_show_char(x, y, *str, fc, bc, size);
        x += size / 2;
        str++;
    }
}

void lcd_show_int_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t fc, uint8_t bc, uint8_t size) {
    uint8_t size_x = size / 2;
    uint8_t enshow = 0;
    for (uint8_t t = 0; t < len; t++) {
        uint32_t power_val = power_of_10(len - t - 1);
        uint32_t temp = (num / power_val) % 10;
        
        if (enshow == 0 && t < len - 1) {
            if (temp == 0) {
                lcd_show_char(x + t * size_x, y, ' ', fc, bc, size);
                continue;
            } else {
                enshow = 1;
            }
        }
        lcd_show_char(x + t * size_x, y, (char)(temp + 48), fc, bc, size);
    }
}

void lcd_draw_checkerboard(void) {
    lcd_clear_screen(0);
    uint16_t cols[3][2] = {{0, 42}, {43, 85}, {86, 127}};
    uint16_t rows[3][2] = {{0, 42}, {43, 85}, {86, 127}};

    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            if ((r + c) % 2 == 0) {
                lcd_fill(cols[c][0], rows[r][0], cols[c][1], rows[r][1], 1);
            }
        }
    }
}

void lcd_draw_split_screen(void) {
    lcd_clear_screen(0);
    lcd_fill(0, 64, 127, 127, 1);
}

void lcd_flush(void) {
    for (uint8_t page = 0; page < LCD_PAGES; page++) {
        lcd_spi_send(0x60 | (page & 0x0F), true);
        lcd_spi_send(0x70 | (page >> 4), true);

        lcd_spi_send(0x04, true);
        lcd_spi_send(55, false);

        lcd_spi_send(0x01, true);

        uint8_t* page_data = &display_buffer[page * LCD_COLS];
        lcd_spi_send_bulk(page_data, LCD_COLS);
    }
}

// ====================== 8. 演示任务 ======================
void lcd_demo_task(void* arg) {
    typedef enum {
        STATE_DEMO = 0,
        STATE_CHECKERBOARD,
        STATE_SPLIT,
        STATE_MAX
    } lcd_state_t;

    lcd_state_t current_state = STATE_DEMO;
    TickType_t last_switch_time = xTaskGetTickCount();
    const TickType_t switch_interval = pdMS_TO_TICKS(5000);

    while (1) {
        if (gpio_get_level(EXIT_KEY_PIN) == 0) {
            ESP_LOGI(TAG, "检测到退出按键，清空屏幕并停止任务");
            lcd_clear_screen(0);
            lcd_flush();
            vTaskDelete(NULL);
        }

        if (xTaskGetTickCount() - last_switch_time >= switch_interval) {
            last_switch_time = xTaskGetTickCount();
            current_state = (lcd_state_t)((current_state + 1) % STATE_MAX);

            switch (current_state) {
                case STATE_DEMO:
                    ESP_LOGI(TAG, "切换至：几何图形与文字演示");
                    lcd_clear_screen(0);
                    lcd_draw_rectangle(0, 0, 127, 127, 1);
                    lcd_draw_circle(80, 40, 20, 1);
                    lcd_draw_line(0, 0, 127, 127, 1);
                    lcd_show_string(10, 60, "Hello", 1, 0, 12);
                    lcd_show_string(10, 80, "P3PLUS LCD", 1, 0, 12);
                    lcd_show_int_num(10, 100, 12345, 5, 1, 0, 12);
                    lcd_flush();
                    break;

                case STATE_CHECKERBOARD:
                    ESP_LOGI(TAG, "切换至：3x3棋盘格");
                    lcd_draw_checkerboard();
                    lcd_flush();
                    break;

                case STATE_SPLIT:
                    ESP_LOGI(TAG, "切换至：上下分屏");
                    lcd_draw_split_screen();
                    lcd_flush();
                    break;

                default:
                    break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ====================== 9. 主函数 ======================
void app_main(void) {
    spi_bus_init();
    lcd_init();
    xTaskCreate(lcd_demo_task, "lcd_demo_task", 4096, NULL, 1, NULL);
    ESP_LOGI(TAG, "P3PLUS LCD演示程序启动完成");
}
