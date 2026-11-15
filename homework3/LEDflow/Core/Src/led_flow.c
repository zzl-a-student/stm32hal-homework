/**
  ******************************************************************************
  * @file    led_flow.c
  * @brief   流水灯状态机实现（不阻塞，靠 HAL_GetTick 时间戳推进）
  ******************************************************************************
  */
#include "led_flow.h"
#include "app_config.h"

static const GPIO_TypeDef *const s_led_port[LED_NUM] = { LED0_PORT, LED1_PORT, LED2_PORT };
static const uint16_t            s_led_pin[LED_NUM]  = { LED0_PIN,  LED1_PIN,  LED2_PIN  };

static LedFlowState_t s_state;
static uint32_t       s_last_tick;

/* 把当前下标输出到 GPIO：同一时刻只点亮一颗 */
static void LedFlow_Apply(void)
{
  for (uint8_t i = 0; i < LED_NUM; i++)
  {
    GPIO_PinState on  = (LED_ACTIVE_LOW ? GPIO_PIN_RESET : GPIO_PIN_SET);
    GPIO_PinState off = (LED_ACTIVE_LOW ? GPIO_PIN_SET   : GPIO_PIN_RESET);
    HAL_GPIO_WritePin((GPIO_TypeDef *)s_led_port[i], s_led_pin[i],
                      (i == s_state.index) ? on : off);
  }
}

/* 按当前方向走一步并刷新 GPIO */
static void LedFlow_Step(void)
{
  if (s_state.forward)
  {
    s_state.index = (uint8_t)((s_state.index + 1U) % LED_NUM);
  }
  else
  {
    s_state.index = (uint8_t)((s_state.index + LED_NUM - 1U) % LED_NUM);
  }
  LedFlow_Apply();
}

void LedFlow_Init(void)
{
  s_state.index    = 0;
  s_state.forward  = 1;
  s_state.running  = 1;
  s_state.step_ms  = STEP_MS_DEFAULT;
  s_last_tick      = HAL_GetTick();
  LedFlow_Apply();
}

void LedFlow_Tick(uint32_t now_ms)
{
  if (!s_state.running)
  {
    return;
  }

  /* 到点才换灯；用 last += step 累加，避免长时间运行后节拍漂移 */
  if ((uint32_t)(now_ms - s_last_tick) >= s_state.step_ms)
  {
    s_last_tick += s_state.step_ms;

    /* 极端情况下（例如被长时间阻塞）追赶节拍，但最多追到当前时刻 */
    if ((uint32_t)(now_ms - s_last_tick) >= s_state.step_ms)
    {
      s_last_tick = now_ms;
    }

    LedFlow_Step();
  }
}

void LedFlow_ToggleDir(void)
{
  s_state.forward = (uint8_t)!s_state.forward;
}

void LedFlow_ToggleRun(void)
{
  s_state.running = (uint8_t)!s_state.running;

  if (s_state.running)
  {
    /* 从暂停恢复到运行：立刻按当前方向走一步，让"已经继续"看得见。
       否则间隔 2000 ms 时要干等 2 秒灯才动，看着像没反应。
       走完这一步后按新的节拍继续。 */
    LedFlow_Step();
    s_last_tick = HAL_GetTick();
  }
}

void LedFlow_SetRunning(uint8_t running)
{
  if (running != s_state.running)
  {
    LedFlow_ToggleRun();
  }
}

void LedFlow_SetForward(uint8_t forward)
{
  s_state.forward = forward ? 1U : 0U;
}

void LedFlow_SetStepMs(uint16_t step_ms)
{
  if (step_ms < STEP_MS_MIN)
  {
    step_ms = STEP_MS_MIN;
  }
  if (step_ms > STEP_MS_MAX)
  {
    step_ms = STEP_MS_MAX;
  }
  s_state.step_ms = step_ms;
}

uint8_t LedFlow_IsRunning(void)  { return s_state.running; }
uint8_t LedFlow_IsForward(void)  { return s_state.forward; }
uint16_t LedFlow_GetStepMs(void) { return s_state.step_ms; }
uint8_t LedFlow_GetIndex(void)   { return s_state.index;   }

const LedFlowState_t *LedFlow_GetState(void)
{
  return &s_state;
}
