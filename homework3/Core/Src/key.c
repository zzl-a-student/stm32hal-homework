/**
  ******************************************************************************
  * @file    key.c
  * @brief   按键消抖：每个按键独立状态机，只有"稳定按下"的那一次产生事件，
  *          按住不放不会重复触发。
  ******************************************************************************
  */
#include "key.h"
#include "app_config.h"

typedef struct
{
  GPIO_TypeDef *port;
  uint16_t      pin;
  KeyEvent_t    event;
  uint8_t       stable;     /* 消抖后的稳定状态：1 = 按下 */
  uint8_t       raw;        /* 最近一次采样值 */
  uint32_t      raw_tick;   /* raw 发生变化的时间 */
} KeyObj_t;

static KeyObj_t s_keys[] =
{
  { KEY_DIR_PORT, KEY_DIR_PIN, KEY_EVENT_DIR, 0, 0, 0 },
  { KEY_RUN_PORT, KEY_RUN_PIN, KEY_EVENT_RUN, 0, 0, 0 },
};

#define KEY_COUNT (sizeof(s_keys) / sizeof(s_keys[0]))

static uint8_t Key_ReadPressed(const KeyObj_t *k)
{
  return (HAL_GPIO_ReadPin(k->port, k->pin) == KEY_PRESSED_LEVEL) ? 1U : 0U;
}

void Key_Init(void)
{
  for (uint32_t i = 0; i < KEY_COUNT; i++)
  {
    s_keys[i].raw      = Key_ReadPressed(&s_keys[i]);
    s_keys[i].stable   = s_keys[i].raw;
    s_keys[i].raw_tick = HAL_GetTick();
  }
}

KeyEvent_t Key_Scan(uint32_t now_ms)
{
  KeyEvent_t ev = KEY_EVENT_NONE;

  for (uint32_t i = 0; i < KEY_COUNT; i++)
  {
    KeyObj_t *k = &s_keys[i];
    uint8_t   pressed = Key_ReadPressed(k);

    if (pressed != k->raw)
    {
      /* 电平变了，重新开始计时（抖动期间会反复刷新，稳定后才生效） */
      k->raw      = pressed;
      k->raw_tick = now_ms;
    }
    else if ((uint32_t)(now_ms - k->raw_tick) >= KEY_DEBOUNCE_MS)
    {
      if (k->stable != k->raw)
      {
        k->stable = k->raw;
        if (k->stable)          /* 只在按下的那一刻出事件 */
        {
          ev = k->event;
        }
      }
    }
    else
    {
      /* 还在消抖窗口内，什么都不做 */
    }
  }

  return ev;
}
