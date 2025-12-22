/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define M2_LOCK_Pin GPIO_PIN_13
#define M2_LOCK_GPIO_Port GPIOC
#define L4_Pin GPIO_PIN_1
#define L4_GPIO_Port GPIOC
#define L5_Pin GPIO_PIN_2
#define L5_GPIO_Port GPIOC
#define L6_Pin GPIO_PIN_3
#define L6_GPIO_Port GPIOC
#define PZTI2_AD_Pin GPIO_PIN_1
#define PZTI2_AD_GPIO_Port GPIOA
#define SW2_Pin GPIO_PIN_4
#define SW2_GPIO_Port GPIOA
#define PZT_V2_Pin GPIO_PIN_5
#define PZT_V2_GPIO_Port GPIOA
#define DA_SCL_Pin GPIO_PIN_6
#define DA_SCL_GPIO_Port GPIOA
#define DA_SDA_Pin GPIO_PIN_7
#define DA_SDA_GPIO_Port GPIOA
#define PZTI1_AD_Pin GPIO_PIN_4
#define PZTI1_AD_GPIO_Port GPIOC
#define DCDC_AD_Pin GPIO_PIN_5
#define DCDC_AD_GPIO_Port GPIOC
#define PZT_V1_Pin GPIO_PIN_0
#define PZT_V1_GPIO_Port GPIOB
#define SW1_Pin GPIO_PIN_2
#define SW1_GPIO_Port GPIOB
#define PWR_L_EN_Pin GPIO_PIN_10
#define PWR_L_EN_GPIO_Port GPIOB
#define PWR_H_EN_Pin GPIO_PIN_12
#define PWR_H_EN_GPIO_Port GPIOB
#define M1_LOCK_Pin GPIO_PIN_14
#define M1_LOCK_GPIO_Port GPIOB
#define LED_Pin GPIO_PIN_7
#define LED_GPIO_Port GPIOC
#define L1_Pin GPIO_PIN_8
#define L1_GPIO_Port GPIOC
#define L2_Pin GPIO_PIN_9
#define L2_GPIO_Port GPIOC
#define L3_Pin GPIO_PIN_8
#define L3_GPIO_Port GPIOA
#define M1_DIR_Pin GPIO_PIN_10
#define M1_DIR_GPIO_Port GPIOA
#define M1_PWM_Pin GPIO_PIN_11
#define M1_PWM_GPIO_Port GPIOA
#define M1_DIAG_Pin GPIO_PIN_15
#define M1_DIAG_GPIO_Port GPIOA
#define M1_UART_Pin GPIO_PIN_10
#define M1_UART_GPIO_Port GPIOC
#define M1_INDEX_Pin GPIO_PIN_11
#define M1_INDEX_GPIO_Port GPIOC
#define TIRGGER_Pin GPIO_PIN_12
#define TIRGGER_GPIO_Port GPIOC
#define M1_ENN_Pin GPIO_PIN_3
#define M1_ENN_GPIO_Port GPIOB
#define M2_PWM_Pin GPIO_PIN_4
#define M2_PWM_GPIO_Port GPIOB
#define M2_DIR_Pin GPIO_PIN_5
#define M2_DIR_GPIO_Port GPIOB
#define M2_UART_Pin GPIO_PIN_6
#define M2_UART_GPIO_Port GPIOB
#define M2_INDEX_Pin GPIO_PIN_7
#define M2_INDEX_GPIO_Port GPIOB
#define M2_DIAG_Pin GPIO_PIN_8
#define M2_DIAG_GPIO_Port GPIOB
#define M2_ENN_Pin GPIO_PIN_9
#define M2_ENN_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define LED_H HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET)
#define LED_L HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET)
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
