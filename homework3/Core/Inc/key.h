/**
  ******************************************************************************
  * @file    key.h
  * @brief   两个按键的消抖扫描（主循环里轮询，按下沿触发一次事件）
  ******************************************************************************
  */
#ifndef __KEY_H
#define __KEY_H

#include <stdint.h>

typedef enum
{
  KEY_EVENT_NONE = 0,
  KEY_EVENT_DIR,      /* 切换正/反向 */
  KEY_EVENT_RUN       /* 切换运行/暂停 */
} KeyEvent_t;

void       Key_Init(void);
KeyEvent_t Key_Scan(uint32_t now_ms);   /* 主循环每轮调用一次 */

#endif /* __KEY_H */
