/**
  ******************************************************************************
  * @file    app_config.h
  * @brief   全工程唯一的硬件/参数配置入口。
  *          换板子、换引脚只改这里 + CubeMX，不用动业务代码。
  ******************************************************************************
  */
#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#include "main.h"

/* ============================ LED 流水灯 ================================= */
/* 3 颗 LED，低电平点亮（PB7/PB8/PB9） */
#define LED_NUM                 3

#define LED0_PORT               GPIOB
#define LED0_PIN                GPIO_PIN_7
#define LED1_PORT               GPIOB
#define LED1_PIN                GPIO_PIN_8
#define LED2_PORT               GPIOB
#define LED2_PIN                GPIO_PIN_9

#define LED_ACTIVE_LOW          1       /* 1: 输出低电平点亮 */

/* ============================== 按键 ==================================== */
/* 两个按键，上拉输入，按下为低电平 */
#define KEY_DIR_PORT            GPIOB   /* 切换正/反向 */
#define KEY_DIR_PIN             GPIO_PIN_13

#define KEY_RUN_PORT            GPIOB   /* 切换运行/暂停 */
#define KEY_RUN_PIN             GPIO_PIN_12

#define KEY_PRESSED_LEVEL       GPIO_PIN_RESET
#define KEY_DEBOUNCE_MS         20U     /* 消抖时间 */

/* ============================ 流水灯参数 ================================ */
#define STEP_MS_DEFAULT         300U    /* 上电默认切换间隔 */
#define STEP_MS_MIN             100U    /* 协议允许的最小值 */
#define STEP_MS_MAX             2000U   /* 协议允许的最大值 */

/* ============================ 串口协议 ================================== */
#define CMD_HEADER              0xAAU   /* 固定帧头 */
#define CMD_PACKET_LEN          6U      /* AA RUN DIR STEP_L STEP_H CHECK */

/* ============================== OLED =================================== */
/* SSD1306 128x64，两种传输方式二选一，引脚都是 PB10/PB11：
   0 = 软件 I²C（作业必做项，两个 GPIO 模拟时序，不依赖 I2C 外设）
   1 = 硬件 I²C2（选做项，走 CubeMX 配好的 hi2c2，100 kHz） */
#define OLED_USE_HW_I2C         1

/* 软件 I²C 的引脚（OLED_USE_HW_I2C = 1 时不生效） */
#define OLED_SCL_PORT           GPIOB
#define OLED_SCL_PIN            GPIO_PIN_10
#define OLED_SDA_PORT           GPIOB
#define OLED_SDA_PIN            GPIO_PIN_11

#endif /* __APP_CONFIG_H */
