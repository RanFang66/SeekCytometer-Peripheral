/*
 * led_control.c
 *
 *  Created on: 2025年12月9日
 *      Author: ranfa
 */

#include "led_control.h"
#include "bsp_gpio.h"

static LedCtrlCtx_t ledCtrl;

#define LED_MCP4728_CHANNEL		MCP4728_CHANNEL_D

void LED_Init()
{
	ledCtrl.gpioEn = POWER_15V_EN_GPIO;
	ledCtrl.pinEn = POWER_15V_EN_PIN;
	ledCtrl.channel = LED_MCP4728_CHANNEL;
	ledCtrl.intensity = 0;
	ledCtrl.status = LED_OFF;

	HAL_GPIO_WritePin(ledCtrl.gpioEn, ledCtrl.pinEn, GPIO_PIN_RESET);
}

void LED_SwitchOff()
{
	MCP4728_Set_Value(ledCtrl.channel, 0);
	HAL_GPIO_WritePin(ledCtrl.gpioEn, ledCtrl.pinEn, GPIO_PIN_RESET);
	ledCtrl.status = LED_OFF;
	ledCtrl.intensity = 0;
}


void LED_SwitchOn(uint16_t val)
{
	val = (val > 4095) ? 4095 : val;
	HAL_GPIO_WritePin(ledCtrl.gpioEn, ledCtrl.pinEn, GPIO_PIN_SET);
	MCP4728_Set_Value(ledCtrl.channel, val);
	ledCtrl.status = LED_ON;
	ledCtrl.intensity = val;
}


void LED_SetIntensity(uint16_t val)
{
	val = (val > 4095) ? 4095 : val;
	MCP4728_Set_Value(ledCtrl.channel, val);
	ledCtrl.intensity = val;
}


uint8_t LED_GetStatus()
{
	return ledCtrl.status;
}

uint16_t LED_GetIntensity()
{
	return ledCtrl.intensity;
}
