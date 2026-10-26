# 电控组第二次培训作业

- **张中伦**
- ***20253003198***

## 版本一：阻塞式

代码思路：每次点亮一个 LED 后调用 `HAL_Delay(500)` 等待，再熄灭并点亮下一个，如此循环。

## 版本二：时间戳

代码思路：版本二改用 `HAL_GetTick()` 读取系统毫秒计数，只判断“到点没有”，不再原地等待。

```c
uint8_t  step = 0;          /* 当前亮第几个灯：0/1/2 */
uint32_t last_tick = 0;     /* 上一次换灯的时间戳 */

while (1)
{
  if (HAL_GetTick() - last_tick >= LED_FLOW_INTERVAL_MS)
  {
    last_tick += LED_FLOW_INTERVAL_MS;

    /* 先全灭 */
    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);

    /* 只点亮当前这个灯 */
    if (step == 0)
    {
      HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET);
    }
    else if (step == 1)
    {
      HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    }
    else
    {
      HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
    }

    /* 0 -> 1 -> 2 -> 0 */
    step++;
    if (step >= 3)
    {
      step = 0;
    }
  }
}
```

## 版本三：FreeRTOS

代码思路：三个任务各负责一个 LED，`osDelay()` 在等待时让出 CPU。三个任务启动时分别延时 0、500、1000 ms，形成相位差，灯就依次点亮，每盏亮 0.5 s。

```c
void led0(void *argument)
{
  for (;;)
  {
    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET);
    osDelay(500);
    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);
    osDelay(1000);
  }
}
```

`led1`、`led2` 结构相同，只是进入循环前分别 `osDelay(500)` 和 `osDelay(1000)`。

> 与版本二的区别：版本二靠主循环反复查询时间，版本三由内核统一调度；等待期间 CPU 进入空闲任务，其它任务可以并行运行。
