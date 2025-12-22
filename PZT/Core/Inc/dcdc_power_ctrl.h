#ifndef __DCDC_POWER_CTRL_H
#define __DCDC_POWER_CTRL_H

#include "main.h"
#include "MCP4728.h"
#define PWR_L_EN_H HAL_GPIO_WritePin(PWR_L_EN_GPIO_Port, PWR_L_EN_Pin, GPIO_PIN_SET)
#define PWR_L_EN_L HAL_GPIO_WritePin(PWR_L_EN_GPIO_Port, PWR_L_EN_Pin, GPIO_PIN_RESET)

#define PWR_H_EN_H HAL_GPIO_WritePin(PWR_H_EN_GPIO_Port, PWR_H_EN_Pin, GPIO_PIN_SET)
#define PWR_H_EN_L HAL_GPIO_WritePin(PWR_H_EN_GPIO_Port, PWR_H_EN_Pin, GPIO_PIN_RESET)

void dcdc_set_power(uint16_t mv);
void dcdc_power_off(void);
#endif
