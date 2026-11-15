/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);

/* USER CODE BEGIN Prototypes */
void    Uart_Init(void);                                  /* 挂上接收中断 */
uint8_t Uart_PopByte(uint8_t *byte);                      /* 主循环取一个收到的字节，1 = 取到 */
void    Uart_SendStatus(uint8_t running, uint8_t forward,
                        uint16_t step_ms, uint8_t led_index);   /* 非阻塞发送状态反馈 */
void    Uart_Task(void);
void    Uart_SendString(const char *str);                 /* 发送调试字符串（<63 字节） */                                  /* 主循环调用：补发挂起的反馈 */
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

