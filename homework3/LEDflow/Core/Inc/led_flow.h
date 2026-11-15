/**
  ******************************************************************************
  * @file    led_flow.h
  * @brief   流水灯状态机：方向、运行/暂停、切换间隔、当前点亮的 LED。
  *          全工程只有这一份流水灯状态，按键和串口都改它。
  ******************************************************************************
  */
#ifndef __LED_FLOW_H
#define __LED_FLOW_H

#include <stdint.h>

typedef struct
{
  uint8_t  index;       /* 当前点亮的 LED 下标 0..LED_NUM-1 */
  uint8_t  forward;     /* 1 = 正向，0 = 反向 */
  uint8_t  running;     /* 1 = 运行，0 = 暂停 */
  uint16_t step_ms;     /* 相邻 LED 切换间隔 */
} LedFlowState_t;

void     LedFlow_Init(void);
void     LedFlow_Tick(uint32_t now_ms);      /* 主循环里反复调用 */
void     LedFlow_ToggleDir(void);
void     LedFlow_ToggleRun(void);
void     LedFlow_SetRunning(uint8_t running);
void     LedFlow_SetForward(uint8_t forward);
void     LedFlow_SetStepMs(uint16_t step_ms);

uint8_t              LedFlow_IsRunning(void);
uint8_t              LedFlow_IsForward(void);
uint16_t             LedFlow_GetStepMs(void);
uint8_t              LedFlow_GetIndex(void);
const LedFlowState_t *LedFlow_GetState(void);

#endif /* __LED_FLOW_H */
