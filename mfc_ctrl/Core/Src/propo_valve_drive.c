/*
 * propo_valve_drive.c
 *
 *  Created on: Oct 22, 2025
 *      Author: ranfa
 */
#include "stdlib.h"
#include "propo_valve_drive.h"
#include "debug_shell.h"
#include "bsp_gpio.h"
#include "bsp_spi.h"
#include "spi.h"


static dac8568_t hdac8568;
static PropoValveDrive_t pValveDrive[PROPO_VALVE_NUM];
// Map：proportional valve 0~4 -> DAC channel A, C, E, D, B
static const dac8568_channel_t kMapValveToCh[PROPO_VALVE_NUM] = {
    DAC8568_CH_A, // 0
    DAC8568_CH_C, // 1
    DAC8568_CH_E, // 2
    DAC8568_CH_D, // 3
    DAC8568_CH_B  // 4
};

static inline int valid_id(uint8_t id) { return id < PROPO_VALVE_NUM; }


HAL_StatusTypeDef  PropoValveDrive_Init()
{
	HAL_StatusTypeDef ret = DAC8568_Init(&hdac8568, &DAC8568_SPI_HANDLE, DAC_SPI_SYNC_GPIO, DAC_SPI_SYNC_PIN, false, 3.3);
	if (ret != HAL_OK) {
		LOG_ERROR("Initial DAC 8568 chip failed!");
		return ret;
	}

	for (uint8_t i = 0; i < PROPO_VALVE_NUM; i++) {
		pValveDrive[i].dacChannel = kMapValveToCh[i];
		pValveDrive[i].shadowValue = 0;
	}

	return HAL_OK;
}


HAL_StatusTypeDef  PropoValveDrive_Set(ValveID_t id, uint16_t value)
{
	if (!valid_id(id)) {
		LOG_ERROR("Wrong proportional valve id %d", id);
		return HAL_ERROR;
	}

	pValveDrive[id].shadowValue = value;
	return DAC8568_WriteInputOnly(&hdac8568, pValveDrive[id].dacChannel, value);
}


HAL_StatusTypeDef  PropoValveDrive_SetAndUpdate(ValveID_t id, uint16_t value)
{
	if (!valid_id(id)) {
		LOG_ERROR("Wrong proportional valve id %d", id);
		return HAL_ERROR;
	}

	pValveDrive[id].shadowValue = value;
	return DAC8568_WriteUpdate(&hdac8568, pValveDrive[id].dacChannel, value);
}

HAL_StatusTypeDef  PropoValveDrive_SetAllAndUpdate(const uint16_t values[PROPO_VALVE_NUM])
{
	if (!values) return HAL_ERROR;

	HAL_StatusTypeDef st = HAL_OK;

	for (int i = 0; i < PROPO_VALVE_NUM; ++i) {
		pValveDrive[i].shadowValue = values[i];
		st = DAC8568_WriteInputOnly(&hdac8568, pValveDrive[i].dacChannel, values[i]);
		if (st != HAL_OK) return st;
	}

	return DAC8568_UpdateAll(&hdac8568);
}


HAL_StatusTypeDef  PropoValveDrive_UpdateAll()
{
	return DAC8568_UpdateAll(&hdac8568);
}

HAL_StatusTypeDef  PropoValveDrive_Close(ValveID_t id)
{
	if (!valid_id(id)) {
		LOG_ERROR("Wrong proportional valve id %d", id);
		return HAL_ERROR;
	}

	pValveDrive[id].shadowValue = 0;

	return DAC8568_WriteUpdate(&hdac8568, pValveDrive[id].dacChannel, 0x0000);
}

HAL_StatusTypeDef  PropoValveDrive_CloseAll()
{
	HAL_StatusTypeDef st = HAL_OK;

	for (int i = 0; i < PROPO_VALVE_NUM; ++i) {
		pValveDrive[i].shadowValue = 0;
		st = DAC8568_WriteInputOnly(&hdac8568, pValveDrive[i].dacChannel, 0x0000);
		if (st != HAL_OK) return st;
	}

	return DAC8568_UpdateAll(&hdac8568);
}


uint16_t PropoValveDrive_GetValue(ValveID_t id)
{
	if (!valid_id(id)) {
		return 0;
	}
	return pValveDrive[id].shadowValue;
}
