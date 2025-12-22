/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, M2_LOCK_Pin|LED_Pin|TIRGGER_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SW2_Pin|M1_DIR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, DA_SCL_Pin|DA_SDA_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, SW1_Pin|PWR_L_EN_Pin|PWR_H_EN_Pin|M1_LOCK_Pin
                          |M1_ENN_Pin|M2_DIR_Pin|M2_ENN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : M2_LOCK_Pin LED_Pin TIRGGER_Pin */
  GPIO_InitStruct.Pin = M2_LOCK_Pin|LED_Pin|TIRGGER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : L4_Pin L5_Pin L6_Pin L1_Pin
                           L2_Pin */
  GPIO_InitStruct.Pin = L4_Pin|L5_Pin|L6_Pin|L1_Pin
                          |L2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : SW2_Pin M1_DIR_Pin */
  GPIO_InitStruct.Pin = SW2_Pin|M1_DIR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : DA_SCL_Pin DA_SDA_Pin */
  GPIO_InitStruct.Pin = DA_SCL_Pin|DA_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : SW1_Pin PWR_L_EN_Pin PWR_H_EN_Pin M1_LOCK_Pin
                           M1_ENN_Pin M2_DIR_Pin M2_ENN_Pin */
  GPIO_InitStruct.Pin = SW1_Pin|PWR_L_EN_Pin|PWR_H_EN_Pin|M1_LOCK_Pin
                          |M1_ENN_Pin|M2_DIR_Pin|M2_ENN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : L3_Pin */
  GPIO_InitStruct.Pin = L3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(L3_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : M1_DIAG_Pin */
  GPIO_InitStruct.Pin = M1_DIAG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(M1_DIAG_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : M1_INDEX_Pin */
  GPIO_InitStruct.Pin = M1_INDEX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(M1_INDEX_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : M2_INDEX_Pin M2_DIAG_Pin */
  GPIO_InitStruct.Pin = M2_INDEX_Pin|M2_DIAG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
