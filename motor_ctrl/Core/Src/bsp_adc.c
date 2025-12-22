/*
 * bsp_adc.c
 *
 *  Created on: 2025年12月2日
 *      Author: ranfa
 */


#include "bsp_adc.h"
#include "debug_shell.h"

static volatile uint16_t adcValues[ADC_ENABLED_CHANNEL_NUM];

static uint16_t adcBuffer[ADC_ENABLED_CHANNEL_NUM][DATA_BUFFER_SIZE];
static uint16_t adcSumBuffer[ADC_ENABLED_CHANNEL_NUM];
static uint16_t adcFiltered[ADC_ENABLED_CHANNEL_NUM];

enum ADC_VALUE_INDEX {
	MOTOR_CURRENT_INDEX = 0,
	LASER2_CURRENT_INDEX,
	LASER2_POWER_INDEX,
	LASER1_POWER_INDEX,
	TEMP_INDEX,
	LASER1_CURRENT_INDEX,
};


//static uint16_t motorIAdc[DATA_BUFFER_SIZE];
//static uint16_t tempAdc[DATA_BUFFER_SIZE];
//static uint16_t laser1IAdc[DATA_BUFFER_SIZE];
//static uint16_t laser1PAdc[DATA_BUFFER_SIZE];
//static uint16_t laser2IAdc[DATA_BUFFER_SIZE];
//static uint16_t laser2PAdc[DATA_BUFFER_SIZE];
//
//static uint16_t motorISum = 0;
//static uint16_t tempSum = 0;
//static uint16_t laser1ISum = 0;
//static uint16_t laser1PSum = 0;
//static uint16_t laser2ISum = 0;
//static uint16_t laser2PSum = 0;

//static uint16_t motorIFiltered = 0;
//static uint16_t tempFiltered = 0;
//static uint16_t laser1IFiltered = 0;
//static uint16_t laser2IFiltered = 0;
//static uint16_t laser1PFiltered = 0;
//static uint16_t laser2PFiltered = 0;

static uint8_t sampleIndex = 0;

void BSP_StartAdcConvertContinuous()
{
	memset(adcBuffer, 0, sizeof(adcBuffer));
	memset(adcSumBuffer, 0, sizeof(adcSumBuffer));
	memset(adcFiltered, 0, sizeof(adcFiltered));
	sampleIndex = 0;

	if (HAL_ADC_Start_DMA(&BSP_ADC_HANDLE, (uint32_t *)adcValues, ADC_ENABLED_CHANNEL_NUM) != HAL_OK) {
		LOG_ERROR("Start ADC conversion FAILED!");
	} else {
		LOG_INFO("Start ADC conversion OK");
	}
}

void BSP_StopAdcConvert(void)
{
	HAL_ADC_Stop_DMA(&BSP_ADC_HANDLE);
}


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	// Update adc value from DMA data
	if (hadc == &BSP_ADC_HANDLE) {
		for (uint8_t i = 0; i < ADC_ENABLED_CHANNEL_NUM; ++i) {
			adcSumBuffer[i] = adcSumBuffer[i] - adcBuffer[i][sampleIndex] + (adcValues[i] & 0x0FFFU);
			adcBuffer[i][sampleIndex] = adcValues[i];
			adcFiltered[i] = adcSumBuffer[i] / DATA_BUFFER_SIZE;
		}
		sampleIndex++;
		if (sampleIndex >= DATA_BUFFER_SIZE)  {
			sampleIndex = 0;
		}
	}
}


uint16_t GetMotorCurrentAdc()
{
	return adcFiltered[MOTOR_CURRENT_INDEX];
}

uint16_t GetTempAdc()
{
	return adcFiltered[TEMP_INDEX];
}

uint16_t GetLaser1CurrentAdc()
{
	return adcFiltered[LASER1_CURRENT_INDEX];
}

uint16_t GetLaser2CurrentAdc()
{
	return adcFiltered[LASER2_CURRENT_INDEX];
}


uint16_t GetLaser1PowerAdc()
{
	return adcFiltered[LASER1_POWER_INDEX];
}


uint16_t GetLaser2PowerAdc()
{
	return adcFiltered[LASER2_POWER_INDEX];
}


