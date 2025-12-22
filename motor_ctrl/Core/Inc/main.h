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
#define M4_DIR_Pin GPIO_PIN_4
#define M4_DIR_GPIO_Port GPIOE
#define M4_EN_Pin GPIO_PIN_6
#define M4_EN_GPIO_Port GPIOE
#define EN_laser2_Pin GPIO_PIN_6
#define EN_laser2_GPIO_Port GPIOA
#define EN_laser1_Pin GPIO_PIN_5
#define EN_laser1_GPIO_Port GPIOC
#define DA_LDAC_Pin GPIO_PIN_11
#define DA_LDAC_GPIO_Port GPIOE
#define DA_SDA_Pin GPIO_PIN_12
#define DA_SDA_GPIO_Port GPIOE
#define DA_SCL_Pin GPIO_PIN_13
#define DA_SCL_GPIO_Port GPIOE
#define EN_15_Pin GPIO_PIN_12
#define EN_15_GPIO_Port GPIOB
#define EN_12V_Pin GPIO_PIN_13
#define EN_12V_GPIO_Port GPIOB
#define M1_Power_Pin GPIO_PIN_7
#define M1_Power_GPIO_Port GPIOC
#define L5_Pin GPIO_PIN_8
#define L5_GPIO_Port GPIOC
#define M2_Power_Pin GPIO_PIN_9
#define M2_Power_GPIO_Port GPIOC
#define M6_DIR_Pin GPIO_PIN_8
#define M6_DIR_GPIO_Port GPIOA
#define M6_FG_Pin GPIO_PIN_9
#define M6_FG_GPIO_Port GPIOA
#define M5_DIR_Pin GPIO_PIN_11
#define M5_DIR_GPIO_Port GPIOA
#define M5_FG_Pin GPIO_PIN_12
#define M5_FG_GPIO_Port GPIOA
#define L6_Pin GPIO_PIN_10
#define L6_GPIO_Port GPIOC
#define L7_Pin GPIO_PIN_11
#define L7_GPIO_Port GPIOC
#define L8_Pin GPIO_PIN_12
#define L8_GPIO_Port GPIOC
#define L9_Pin GPIO_PIN_0
#define L9_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
