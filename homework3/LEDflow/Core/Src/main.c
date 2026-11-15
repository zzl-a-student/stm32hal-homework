/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app_config.h"
#include "led_flow.h"
#include "key.h"
#include "protocol.h"
#include "oled.h"
#include "font.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint8_t s_disp_dirty = 1;      /* 状态变了，需要重画一帧 */
static uint8_t s_last_index = 0xFFU;  /* 上一次显示的 LED 下标 */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void App_ShowStatus(void);
static void App_SendStatus(void);
static void App_ReportOled(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* 把当前状态画到 OLED：运行状态 / 方向 / 间隔 / 当前 LED 编号 */
static void App_ShowStatus(void)
{
  char line[24];

  OLED_NewFrame();                       /* 清显存 */

  OLED_PrintASCIIString(0, 0, "LED FLOW CTRL", &afont12x6, OLED_COLOR_NORMAL);

  snprintf(line, sizeof(line), "STATE: %s", LedFlow_IsRunning() ? "RUN" : "PAUSE");
  OLED_PrintASCIIString(0, 13, line, &afont12x6, OLED_COLOR_NORMAL);

  snprintf(line, sizeof(line), "DIR  : %s", LedFlow_IsForward() ? "FWD" : "REV");
  OLED_PrintASCIIString(0, 26, line, &afont12x6, OLED_COLOR_NORMAL);

  snprintf(line, sizeof(line), "STEP : %ums", (unsigned)LedFlow_GetStepMs());
  OLED_PrintASCIIString(0, 39, line, &afont12x6, OLED_COLOR_NORMAL);

  snprintf(line, sizeof(line), "LED  : %u", (unsigned)(LedFlow_GetIndex() + 1U));
  OLED_PrintASCIIString(0, 52, line, &afont12x6, OLED_COLOR_NORMAL);

  OLED_ShowFrameDirty();                 /* 只发变化的页 */
}

/* 上电自检结果：屏有没有应答、地址是多少，用串口打出来方便排查接线 */
static void App_ReportOled(void)
{
  char     msg[64];
  int      len;

  if (OLED_IsReady())
  {
    len = snprintf(msg, sizeof(msg), "OLED:OK addr=0x%02X\r\n",
                   (unsigned)(OLED_GetAddr() >> 1));
  }
  else
  {
    uint8_t  found[8];
    uint16_t n = OLED_ScanBus(found, sizeof(found));

    len = snprintf(msg, sizeof(msg), "OLED:FAIL, ACK:");
    if (n == 0)
    {
      len += snprintf(msg + len, sizeof(msg) - (size_t)len, " none\r\n");
    }
    else
    {
      for (uint16_t i = 0; i < n && i < sizeof(found) && len < (int)sizeof(msg) - 6; i++)
      {
        len += snprintf(msg + len, sizeof(msg) - (size_t)len, " 0x%02X",
                        (unsigned)(found[i] >> 1));
      }
      len += snprintf(msg + len, sizeof(msg) - (size_t)len, "\r\n");
    }
  }

  if (len > 0)
  {
    Uart_SendString(msg);
  }
}

/* 串口反馈：把当前这一份状态回传给上位机 */
static void App_SendStatus(void)
{
  Uart_SendStatus(LedFlow_IsRunning(), LedFlow_IsForward(),
                      LedFlow_GetStepMs(), LedFlow_GetIndex());
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */
  LedFlow_Init();          /* 流水灯状态 + GPIO 先落地 */
  Key_Init();
  OLED_Init();             /* 软件 I²C 驱动 SSD1306 */
  App_ShowStatus();
  Proto_Reset();
  Uart_Init();         /* 打开串口接收中断 */
  App_ReportOled();    /* 上电打一行 OLED 自检结果 */

  App_SendStatus();        /* 上电先回一帧当前状态 */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t   now = HAL_GetTick();
    uint8_t    rx_byte;
    KeyEvent_t key_ev;

    /* 1. 按键：消抖后的一次按下 = 一次操作 */
    key_ev = Key_Scan(now);
    if (key_ev == KEY_EVENT_DIR)
    {
      LedFlow_ToggleDir();
      s_disp_dirty = 1;
      App_SendStatus();
    }
    else if (key_ev == KEY_EVENT_RUN)
    {
      LedFlow_ToggleRun();
      s_disp_dirty = 1;
      App_SendStatus();
    }
    else
    {
      /* 无按键事件 */
    }

    /* 2. 串口：中断只负责收字节，这里解析、校验、更新状态 */
    while (Uart_PopByte(&rx_byte))
    {
      CmdPacket_t   pkt;
      ProtoResult_t res = Proto_Feed(rx_byte, &pkt);

      if (res == PROTO_OK)
      {
        LedFlow_SetRunning(pkt.run);
        LedFlow_SetForward(pkt.dir);
        LedFlow_SetStepMs(Proto_GetStepMs(&pkt));
        s_disp_dirty = 1;
        App_SendStatus();
      }
      /* 帧头/校验/字段不对：整包丢弃，不改任何状态 */
    }

    /* 3. 流水灯节拍 */
    LedFlow_Tick(now);

    /* 4. 串口反馈补发（上一帧还在发的时候挂起的） */
    Uart_Task();

    /* 5. 状态变化或换灯时刷新 OLED，保证编号与实际点亮的一致 */
    if (s_disp_dirty || LedFlow_GetIndex() != s_last_index)
    {
      s_last_index = LedFlow_GetIndex();
      s_disp_dirty = 0;
      App_ShowStatus();
    }

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
