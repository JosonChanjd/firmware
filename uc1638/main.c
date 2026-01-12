/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "gpio.h"
// 引入 LCD 驱动
#include "uc1638.h"

int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_SPI1_Init(); // 确保这里初始化了 SPI1

  /* USER CODE BEGIN 2 */
  // 1. 初始化 LCD
  UC1638_Init();

  int state = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // 2. 状态机演示逻辑
      switch (state) {
          case 0: // 状态 1: 几何图形与文字
              UC1638_Clear(COLOR_WHITE);
              // 边框
              UC1638_DrawRectangle(0, 0, 127, 127, COLOR_BLACK);
              // 圆
              UC1638_DrawCircle(80, 40, 20, COLOR_BLACK);
              // 对角线
              UC1638_DrawLine(0, 0, 127, 127, COLOR_BLACK);
              // 文字
              UC1638_ShowString(10, 60, "Hello", COLOR_BLACK);
              UC1638_ShowString(10, 80, "P3PLUS LCD", COLOR_BLACK);
              // 数字
              UC1638_ShowInt(10, 100, 12345, 5, COLOR_BLACK);
              break;

          case 1: // 状态 2: 棋盘格
              UC1638_Demo_Checkerboard();
              break;

          case 2: // 状态 3: 上下分屏
              UC1638_Demo_SplitScreen();
              break;
      }

      // 3. 将显存刷新到 LCD 屏幕
      UC1638_Flush();

      // 4. 切换状态
      state = (state + 1) % 3;
      
      // 5. 延时 5 秒
      HAL_Delay(5000);
      
    /* USER CODE END WHILE */
  }
}
