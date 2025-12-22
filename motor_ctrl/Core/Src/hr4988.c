/*
 * hr4988.c
 *
 *  Created on: 2025年11月27日
 *      Author: ranfa
 */

#include "hr4988.h"
#include "debug_shell.h"

void HR4988_Init(HR4988_TypeDef *hr4988,
		GPIO_TypeDef *gpioEn, uint16_t pinEn, GPIO_TypeDef *gpioDir, uint16_t pinDir,
		TIM_HandleTypeDef *htim, uint32_t pwmCh)
{
	if (hr4988 == NULL || gpioEn == NULL || gpioDir == NULL || htim == NULL) {
		LOG_ERROR("Initialize HR4988 Failed! Invalid parameter input");
		return;
	}

	hr4988->gpioEn = gpioEn;
	hr4988->pinEn = pinEn;
	hr4988->gpioDir = gpioDir;
	hr4988->pinDir = pinDir;
	hr4988->htim = htim;
	hr4988->pwmChannel = pwmCh;
	hr4988->status = HR4988_DISABLED;
	hr4988->dir = HR4988_DIR_CW;
	hr4988->freq = HR4988_DEFAULT_FREQ;

	// Disable Power
	HAL_GPIO_WritePin(hr4988->gpioEn, hr4988->pinEn, GPIO_PIN_SET);
	// Set direction to CW
	HAL_GPIO_WritePin(hr4988->gpioDir, hr4988->pinDir, GPIO_PIN_SET);

	HAL_TIM_PWM_Stop(hr4988->htim, hr4988->pwmChannel);


	// Set default frequency
	uint32_t timerClock = HAL_RCC_GetPCLK2Freq();
	uint32_t prescaler = hr4988->htim->Init.Prescaler;
	uint32_t period = (timerClock / (prescaler+1) / HR4988_DEFAULT_FREQ) - 1;


	__HAL_TIM_SET_AUTORELOAD(hr4988->htim, period);
	__HAL_TIM_SET_COMPARE(hr4988->htim, hr4988->pwmChannel, (period+1) / 2);

	const osMutexAttr_t attr = {.name = "hr4988"};
	hr4988->lock = osMutexNew(&attr);
	if (hr4988->lock == NULL) {
		LOG_ERROR("Create Mutex lock for HR4988 failed!");
	}
}

void HR4988_Enable(HR4988_TypeDef *hr4988)
{
	osMutexAcquire(hr4988->lock, osWaitForever);
	HAL_GPIO_WritePin(hr4988->gpioEn, hr4988->pinEn, GPIO_PIN_RESET);
	if (hr4988->status == HR4988_DISABLED) {
		hr4988->status = HR4988_IDLE;
	}
	osMutexRelease(hr4988->lock);
}


void HR4988_Disable(HR4988_TypeDef *hr4988)
{
	osMutexAcquire(hr4988->lock, osWaitForever);
	HAL_TIM_PWM_Stop(hr4988->htim, hr4988->pwmChannel);
	HAL_GPIO_WritePin(hr4988->gpioEn, hr4988->pinEn, GPIO_PIN_SET);
	hr4988->status = HR4988_DISABLED;
	osMutexRelease(hr4988->lock);
}

void HR4988_SetDirection(HR4988_TypeDef *hr4988, uint8_t dir)
{
	osMutexAcquire(hr4988->lock, osWaitForever);
	if (hr4988->status == HR4988_RUN_CW && dir == HR4988_DIR_CCW) {
		HAL_TIM_PWM_Stop(hr4988->htim, hr4988->pwmChannel);	// Stop first;
		osDelay(10);
		HAL_GPIO_WritePin(hr4988->gpioDir, hr4988->pinDir, GPIO_PIN_RESET);
		HAL_TIM_PWM_Start(hr4988->htim, hr4988->pwmChannel);
		hr4988->status = HR4988_RUN_CCW;
	} else if (hr4988->status == HR4988_RUN_CCW && dir == HR4988_DIR_CW) {
		HAL_TIM_PWM_Stop(hr4988->htim, hr4988->pwmChannel);	// Stop first;
		osDelay(10);
		HAL_GPIO_WritePin(hr4988->gpioDir, hr4988->pinDir, GPIO_PIN_SET);
		HAL_TIM_PWM_Start(hr4988->htim, hr4988->pwmChannel);
		hr4988->status = HR4988_RUN_CW;
	} else {
		if (dir == HR4988_DIR_CW) {
			HAL_GPIO_WritePin(hr4988->gpioDir, hr4988->pinDir, GPIO_PIN_SET);
		} else {
			HAL_GPIO_WritePin(hr4988->gpioDir, hr4988->pinDir, GPIO_PIN_RESET);
		}
	}
	osMutexRelease(hr4988->lock);
}


void HR4988_SetFreq(HR4988_TypeDef *hr4988, uint32_t freq)
{
	osMutexAcquire(hr4988->lock, osWaitForever);

	uint32_t timerClock = HAL_RCC_GetPCLK2Freq();
	uint32_t prescaler = hr4988->htim->Init.Prescaler;
	uint32_t period = (timerClock / (prescaler+1) / freq) - 1;

	if (period < 10) {
		period = 10;
	}

	__HAL_TIM_SET_AUTORELOAD(hr4988->htim, period);
	__HAL_TIM_SET_COMPARE(hr4988->htim, hr4988->pwmChannel, (period+1) / 2);

	osMutexRelease(hr4988->lock);
}

void HR4988_RunCW(HR4988_TypeDef *hr4988, uint16_t freq)
{
	osMutexAcquire(hr4988->lock, osWaitForever);

	uint32_t timerClock = HAL_RCC_GetPCLK2Freq();
	uint32_t prescaler = hr4988->htim->Init.Prescaler;
	uint32_t period = (timerClock / (prescaler+1) / freq) - 1;
	if (period < 10) {
		period = 10;
	}

	switch (hr4988->status) {
	case HR4988_DISABLED:
		// Enable first
		HAL_GPIO_WritePin(hr4988->gpioEn, hr4988->pinEn, GPIO_PIN_RESET);
		// Set direction
		HAL_GPIO_WritePin(hr4988->gpioDir, hr4988->pinDir, GPIO_PIN_SET);
		// Set frequency
		__HAL_TIM_SET_AUTORELOAD(hr4988->htim, period);
		__HAL_TIM_SET_COMPARE(hr4988->htim, hr4988->pwmChannel, (period+1) / 2);
		// Start pwm
		HAL_TIM_PWM_Start(hr4988->htim, hr4988->pwmChannel);
		break;

	case HR4988_IDLE:
		// Set direction
		HAL_GPIO_WritePin(hr4988->gpioDir, hr4988->pinDir, GPIO_PIN_SET);
		// Set frequency
		__HAL_TIM_SET_AUTORELOAD(hr4988->htim, period);
		__HAL_TIM_SET_COMPARE(hr4988->htim, hr4988->pwmChannel, (period+1) / 2);
		// Start pwm
		HAL_TIM_PWM_Start(hr4988->htim, hr4988->pwmChannel);
		break;

	case HR4988_RUN_CW:
		// Stop first
		HAL_TIM_PWM_Stop(hr4988->htim, hr4988->pwmChannel);
		osDelay(10);
		// Set frequency
		__HAL_TIM_SET_AUTORELOAD(hr4988->htim, period);
		__HAL_TIM_SET_COMPARE(hr4988->htim, hr4988->pwmChannel, (period+1) / 2);
		// Start pwm
		HAL_TIM_PWM_Start(hr4988->htim, hr4988->pwmChannel);
		break;

	case HR4988_RUN_CCW:
		// Stop first
		HAL_TIM_PWM_Stop(hr4988->htim, hr4988->pwmChannel);
		osDelay(10);
		// Set direction
		HAL_GPIO_WritePin(hr4988->gpioDir, hr4988->pinDir, GPIO_PIN_SET);
		// Set frequency
		__HAL_TIM_SET_AUTORELOAD(hr4988->htim, period);
		__HAL_TIM_SET_COMPARE(hr4988->htim, hr4988->pwmChannel, (period+1) / 2);
		// Start pwm
		HAL_TIM_PWM_Start(hr4988->htim, hr4988->pwmChannel);
		break;

	default:
		HAL_TIM_PWM_Stop(hr4988->htim, hr4988->pwmChannel);
		HAL_GPIO_WritePin(hr4988->gpioEn, hr4988->pinEn, GPIO_PIN_SET);
		break;
	}

	hr4988->status = HR4988_RUN_CW;
	hr4988->dir = HR4988_DIR_CW;
	hr4988->freq = freq;

	osMutexRelease(hr4988->lock);
}

void HR4988_RunCCW(HR4988_TypeDef *hr4988, uint16_t freq)
{
	osMutexAcquire(hr4988->lock, osWaitForever);
	uint32_t timerClock = HAL_RCC_GetPCLK2Freq();
	uint32_t prescaler = hr4988->htim->Init.Prescaler;
	uint32_t period = (timerClock / (prescaler+1) / freq) - 1;
	if (period < 10) {
		period = 10;
	}

	switch (hr4988->status) {
	case HR4988_DISABLED:
		// Enable first
		HAL_GPIO_WritePin(hr4988->gpioEn, hr4988->pinEn, GPIO_PIN_RESET);
		// Set direction
		HAL_GPIO_WritePin(hr4988->gpioDir, hr4988->pinDir, GPIO_PIN_RESET);
		// Set frequency
		__HAL_TIM_SET_AUTORELOAD(hr4988->htim, period);
		__HAL_TIM_SET_COMPARE(hr4988->htim, hr4988->pwmChannel, (period+1) / 2);
		// Start pwm
		HAL_TIM_PWM_Start(hr4988->htim, hr4988->pwmChannel);
		break;

	case HR4988_IDLE:
		// Set direction
		HAL_GPIO_WritePin(hr4988->gpioDir, hr4988->pinDir, GPIO_PIN_RESET);
		// Set frequency
		__HAL_TIM_SET_AUTORELOAD(hr4988->htim, period);
		__HAL_TIM_SET_COMPARE(hr4988->htim, hr4988->pwmChannel, (period+1) / 2);
		// Start pwm
		HAL_TIM_PWM_Start(hr4988->htim, hr4988->pwmChannel);
		break;

	case HR4988_RUN_CCW:
		// Stop first
		HAL_TIM_PWM_Stop(hr4988->htim, hr4988->pwmChannel);
		osDelay(10);
		// Set frequency
		__HAL_TIM_SET_AUTORELOAD(hr4988->htim, period);
		__HAL_TIM_SET_COMPARE(hr4988->htim, hr4988->pwmChannel, (period+1) / 2);
		// Start pwm
		HAL_TIM_PWM_Start(hr4988->htim, hr4988->pwmChannel);
		break;

	case HR4988_RUN_CW:
		// Stop first
		HAL_TIM_PWM_Stop(hr4988->htim, hr4988->pwmChannel);
		osDelay(10);
		// Set direction
		HAL_GPIO_WritePin(hr4988->gpioDir, hr4988->pinDir, GPIO_PIN_RESET);
		// Set frequency
		__HAL_TIM_SET_AUTORELOAD(hr4988->htim, period);
		__HAL_TIM_SET_COMPARE(hr4988->htim, hr4988->pwmChannel, (period+1) / 2);
		// Start pwm
		HAL_TIM_PWM_Start(hr4988->htim, hr4988->pwmChannel);
		break;

	default:
		HAL_TIM_PWM_Stop(hr4988->htim, hr4988->pwmChannel);
		HAL_GPIO_WritePin(hr4988->gpioEn, hr4988->pinEn, GPIO_PIN_SET);
		break;
	}

	hr4988->status = HR4988_RUN_CCW;
	hr4988->dir = HR4988_DIR_CCW;
	hr4988->freq = freq;

	osMutexRelease(hr4988->lock);
}


