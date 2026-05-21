/*
 * bsp_adc.h
 *
 *  Created on: 2025年12月2日
 *      Author: ranfa
 */

#ifndef INC_BSP_ADC_H_
#define INC_BSP_ADC_H_

#include "adc.h"

#define BSP_ADC_HANDLE				hadc1
#define DATA_BUFFER_SIZE 			(4)
#define ADC_ENABLED_CHANNEL_NUM		(9)

#define SEAL_MOTOR_CURRENT_CHANNEL	ADC_CHANNEL_4
#define LASER2_CURRENT_CHANNEL		ADC_CHANNEL_5
#define LASER2_POWER_CHANNEL		ADC_CHANNEL_7
#define LASER1_POWER_CHANNEL		ADC_CHANNEL_8
#define NTC_TEMP_CHANNEL			ADC_CHANNEL_9
#define LASER1_CURRENT_CHANNEL		ADC_CHANNEL_14
#define NTC_TEMP_2_CHANNEL			ADC_CHANNEL_3
#define NTC_TEMP_3_CHANNEL			ADC_CHANNEL_2
#define NTC_TEMP_4_CHANNEL			ADC_CHANNEL_13



void BSP_StartAdcConvertContinuous();
void BSP_StopAdcConvert(void);

uint16_t GetMotorCurrentAdc();
uint16_t GetTempAdc_NTC1();
uint16_t GetTempAdc_NTC2();
uint16_t GetTempAdc_NTC3();
uint16_t GetTempAdc_NTC4();


uint16_t GetLaser1CurrentAdc();
uint16_t GetLaser1PowerAdc();
uint16_t GetLaser2CurrentAdc();
uint16_t GetLaser2PowerAdc();



#endif /* INC_BSP_ADC_H_ */
