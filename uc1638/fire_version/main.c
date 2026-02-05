/* ============================================================================== */
/*                                10. 中断服务与系统 MSP (替代 it.c 和 msp.c)     */
/* ============================================================================== */

/**
  * @brief  Cortex-M 系统滴答定时器中断
  *         HAL_Delay() 依赖此中断维持心跳
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
}

/**
  * @brief  DMA1 Stream4 中断处理函数 (用于 SPI2 TX)
  *         必须存在，否则 DMA 完成后无法触发回调
  */
void DMA1_Stream4_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_spi2_tx);
}

/**
  * @brief  SPI2 全局中断处理函数
  *         虽然主要用 DMA，但保留此入口以防错误处理或后续扩展
  */
void SPI2_IRQHandler(void)
{
  HAL_SPI_IRQHandler(&hspi2);
}

/**
  * @brief  初始化全局 MSP (MCU Support Package)
  *         被 HAL_Init() 调用，用于设置中断优先级分组
  */
void HAL_MspInit(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  /* 设置中断优先级分组为 4 (4位抢占，0位响应) */
  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* 系统异常中断优先级设置 (根据需要调整，通常保持默认) */
  HAL_NVIC_SetPriority(MemoryManagement_IRQn, 0, 0);
  HAL_NVIC_SetPriority(BusFault_IRQn, 0, 0);
  HAL_NVIC_SetPriority(UsageFault_IRQn, 0, 0);
  HAL_NVIC_SetPriority(SVCall_IRQn, 0, 0);
  HAL_NVIC_SetPriority(DebugMonitor_IRQn, 0, 0);
  HAL_NVIC_SetPriority(PendSV_IRQn, 0, 0);
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* ============================================================================== */
/*                    关于 HAL_SPI_MspInit 的说明                                 */
/* ============================================================================== */
/* 
   我们已经在 main.c 的 BSP_Init_SPI_DMA 中手动完成了 GPIO 和 DMA 的初始化。
   因此，不需要再定义 HAL_SPI_MspInit。
   当 HAL_SPI_Init 尝试调用弱定义的 HAL_SPI_MspInit 时，会执行空操作，
   这正是我们想要的（因为我们已经手动初始化过了）。
*/
