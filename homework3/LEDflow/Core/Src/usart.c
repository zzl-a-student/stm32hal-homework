/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */
#include <stdio.h>
#include <string.h>

#define RX_RING_SIZE   64U

static uint8_t           s_rx_byte;                  /* Receive_IT 的落点 */
static volatile uint8_t  s_ring[RX_RING_SIZE];
static volatile uint16_t s_head;                     /* 中断写 */
static volatile uint16_t s_tail;                     /* 主循环读 */

/* --- 发送：DMA 发送 + 忙时挂起最新一帧 --- */
static char              s_tx_buf[64];               /* 正在发送的内容，发完前不能改 */
static char              s_pending_buf[64];          /* 发送忙时暂存的最新一帧 */
static uint16_t          s_pending_len;
static volatile uint8_t  s_tx_busy;
static volatile uint8_t  s_tx_pending;
/* USER CODE END 0 */

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_tx;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 DMA Init */
    /* USART1_TX Init */
    hdma_usart1_tx.Instance = DMA1_Channel4;
    hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart1_tx);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

    /* USART1 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* 重新挂上单字节接收中断 */
static void Uart_RxArm(void)
{
  (void)HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1);
}

void Uart_Init(void)
{
  s_head = s_tail = 0;
  s_tx_busy = s_tx_pending = 0;
  Uart_RxArm();
}

uint8_t Uart_PopByte(uint8_t *byte)
{
  if (s_head == s_tail)
  {
    return 0;
  }
  *byte  = s_ring[s_tail];
  s_tail = (uint16_t)((s_tail + 1U) % RX_RING_SIZE);
  return 1;
}

/* 启动一次 DMA 发送；内容先拷进 s_tx_buf，传输完成前不会被改写
   DMA 通道由 CubeMX 配好：DMA1_Channel4 / USART1_TX，循环模式关、字节对齐 */
static void Uart_StartTx(const char *data, uint16_t len)
{
  memcpy(s_tx_buf, data, len);
  s_tx_busy = 1;

  if (HAL_UART_Transmit_DMA(&huart1, (uint8_t *)s_tx_buf, len) != HAL_OK)
  {
    s_tx_busy = 0;                 /* 启动失败就放弃这一帧，等下一帧 */
  }
}

/* 反馈格式：STATE=RUN,DIR=FWD,STEP=300,LED=1 */
/* 发送任意字符串（调试信息用），长度不要超过 63 字节 */
void Uart_SendString(const char *str)
{
  size_t len = strlen(str);

  if (len == 0U)
  {
    return;
  }
  if (len > sizeof(s_tx_buf) - 1U)
  {
    len = sizeof(s_tx_buf) - 1U;
  }

  if (!s_tx_busy)
  {
    Uart_StartTx(str, (uint16_t)len);
  }
  else
  {
    /* 上一帧还没发完：只保留最新的一帧，发完立刻补发 */
    memcpy(s_pending_buf, str, len);
    s_pending_len = (uint16_t)len;
    s_tx_pending  = 1;
  }
}

void Uart_SendStatus(uint8_t running, uint8_t forward,
                     uint16_t step_ms, uint8_t led_index)
{
  char frame[64];
  int  len;

  len = snprintf(frame, sizeof(frame), "STATE=%s,DIR=%s,STEP=%u,LED=%u\r\n",
                 running ? "RUN" : "PAUSE",
                 forward ? "FWD" : "REV",
                 (unsigned)step_ms,
                 (unsigned)(led_index + 1U));   /* LED 编号从 1 开始 */

  if (len <= 0)
  {
    return;
  }
  if (len > (int)sizeof(frame) - 1)
  {
    len = (int)sizeof(frame) - 1;
  }

  Uart_SendString(frame);
}

void Uart_Task(void)
{
  if (!s_tx_busy && s_tx_pending)
  {
    char     tmp[64];
    uint16_t len = s_pending_len;

    s_tx_pending = 0;
    memcpy(tmp, s_pending_buf, len);
    Uart_StartTx(tmp, len);
  }
}

/* ---------------------------- HAL 回调 ---------------------------- */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance != USART1)
  {
    return;
  }

  uint16_t next = (uint16_t)((s_head + 1U) % RX_RING_SIZE);
  if (next != s_tail)                /* 满了就丢新字节，不覆盖未处理数据 */
  {
    s_ring[s_head] = s_rx_byte;
    s_head = next;
  }

  Uart_RxArm();                      /* 继续收下一个字节 */
}

/* DMA 发完最后一个字节后，HAL 的 UART_DMATransmitCplt 会回调这里。
   这里只记录"发完了"这一个事件，真正要不要接着发由主循环的 Uart_Task() 决定，
   符合"中断只做必要的事件记录"的要求。 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance != USART1)
  {
    return;
  }
  s_tx_busy = 0;
}

/* 溢出/噪声等错误后必须重新挂接收，否则串口再也收不到东西 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance != USART1)
  {
    return;
  }
  __HAL_UART_CLEAR_OREFLAG(huart);
  Uart_RxArm();
}

/* USER CODE END 1 */

