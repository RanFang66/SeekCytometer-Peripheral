/*
 * laser_control.c
 *
 *  Created on: 2025年12月9日
 *      Author: ranfa
 */

#include "laser_control.h"
#include "bsp_gpio.h"


#define LASER_1_MCP_CHANNEL	MCP4728_CHANNEL_C
#define LASER_2_MCP_CHANNEL	MCP4728_CHANNEL_B

static LaserCtrlCtx_t laserCtrl[LASER_NUM];


static void InitLaser(LaserIndex_t id, MCP4728_Channel_t ch, GPIO_TypeDef *gpioEn, uint16_t pin)
{
	if (id < 0 || id >= LASER_NUM) {
		return;
	}

	laserCtrl[id].channel = ch;
	laserCtrl[id].gpioEn = gpioEn;
	laserCtrl[id].pinEn = pin;
	laserCtrl[id].status = LASER_OFF;
	laserCtrl[id].intensity = 0;

	// Disable first
	HAL_GPIO_WritePin(gpioEn, pin, GPIO_PIN_SET);
}


void Laser_Init()
{
	InitLaser(LASER_1, LASER_1_MCP_CHANNEL, LASER_1_EN_GPIO, LASER_1_EN_PIN);
	InitLaser(LASER_2, LASER_2_MCP_CHANNEL, LASER_2_EN_GPIO, LASER_2_EN_PIN);
}

void Laser_SwitchOff(LaserIndex_t id)
{
	if (id == LASER_1 || id == LASER_2) {
		MCP4728_Set_Value(laserCtrl[id].channel, 0);
		HAL_GPIO_WritePin(laserCtrl[id].gpioEn, laserCtrl[id].pinEn, GPIO_PIN_SET);
		laserCtrl[id].intensity = 0;
		laserCtrl[id].status = LASER_OFF;
	} else if (id == LASER_ALL) {
		MCP4728_Set_Value(laserCtrl[0].channel, 0);
		HAL_GPIO_WritePin(laserCtrl[0].gpioEn, laserCtrl[0].pinEn, GPIO_PIN_SET);
		laserCtrl[0].intensity = 0;
		laserCtrl[0].status = LASER_OFF;

		MCP4728_Set_Value(laserCtrl[1].channel, 0);
		HAL_GPIO_WritePin(laserCtrl[1].gpioEn, laserCtrl[1].pinEn, GPIO_PIN_SET);
		laserCtrl[1].intensity = 0;
		laserCtrl[1].status = LASER_OFF;
	}
}

void Laser_SwitchOn(LaserIndex_t id, uint16_t val)
{
	val = (val > 4095) ? 4095 : val;

	if (id == LASER_1 || id == LASER_2) {
		HAL_GPIO_WritePin(laserCtrl[id].gpioEn, laserCtrl[id].pinEn, GPIO_PIN_RESET);
		MCP4728_Set_Value(laserCtrl[id].channel, val);
		laserCtrl[id].status = LASER_ON;
		laserCtrl[id].intensity = val;
	} else if (id == LASER_ALL) {
		HAL_GPIO_WritePin(laserCtrl[0].gpioEn, laserCtrl[0].pinEn, GPIO_PIN_RESET);
		MCP4728_Set_Value(laserCtrl[0].channel, val);
		laserCtrl[0].status = LASER_ON;
		laserCtrl[0].intensity = val;

		HAL_GPIO_WritePin(laserCtrl[1].gpioEn, laserCtrl[1].pinEn, GPIO_PIN_RESET);
		MCP4728_Set_Value(laserCtrl[1].channel, val);
		laserCtrl[1].status = LASER_ON;
		laserCtrl[1].intensity = val;
	}
}

void Laser_SetIntensity(LaserIndex_t id, uint16_t val)
{
	val = (val > 4095) ? 4095 : val;

	if (id == LASER_1 || id == LASER_2) {
		MCP4728_Set_Value(laserCtrl[id].channel, val);
		laserCtrl[id].intensity = val;
	} else if (id == LASER_ALL) {
		MCP4728_Set_Value(laserCtrl[0].channel, val);
		laserCtrl[0].intensity = val;

		MCP4728_Set_Value(laserCtrl[1].channel, val);
		laserCtrl[1].intensity = val;
	}
}

uint8_t Laser_GetStatus(LaserIndex_t id)
{
	if (id != LASER_1 && id != LASER_2) {
		return;
	}
	return laserCtrl[id].status;
}

uint16_t Laser_GetIntensity(LaserIndex_t id)
{
	if (id != LASER_1 && id != LASER_2) {
		return;
	}
	return laserCtrl[id].intensity;
}
