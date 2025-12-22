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
#define SS6_Pin GPIO_PIN_1
#define SS6_GPIO_Port GPIOA
#define SYNC_Pin GPIO_PIN_12
#define SYNC_GPIO_Port GPIOB
#define SS1_Pin GPIO_PIN_8
#define SS1_GPIO_Port GPIOC
#define SS2_Pin GPIO_PIN_9
#define SS2_GPIO_Port GPIOC
#define SS3_Pin GPIO_PIN_8
#define SS3_GPIO_Port GPIOA
#define SS4_Pin GPIO_PIN_3
#define SS4_GPIO_Port GPIOB
#define SS5_Pin GPIO_PIN_4
#define SS5_GPIO_Port GPIOB
#define sole_v1_Pin GPIO_PIN_5
#define sole_v1_GPIO_Port GPIOB
#define sole_v2_Pin GPIO_PIN_6
#define sole_v2_GPIO_Port GPIOB
#define sole_v3_Pin GPIO_PIN_7
#define sole_v3_GPIO_Port GPIOB
#define sole_v4_Pin GPIO_PIN_8
#define sole_v4_GPIO_Port GPIOB
#define sole_v5_Pin GPIO_PIN_9
#define sole_v5_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
