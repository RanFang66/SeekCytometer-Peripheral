/*
 * dc_motor.c
 *
 *  Created on: 2025年11月28日
 *      Author: ranfa
 */


#include "dc_motor.h"
#include "cmsis_os2.h"

#define MAX_PWM_VAL			10000


void DCMotor_Init(DCMotor_t *motor, TIM_HandleTypeDef *htim, uint32_t ch, GPIO_TypeDef* gpioEn, uint16_t pinEn, GPIO_TypeDef *gpioDir, uint16_t pinDir)
{
	if (motor == NULL || htim == NULL || gpioEn == NULL || gpioDir == NULL) {
		return;
	}

	motor->htim = htim;
	motor->pwmCh = ch;
	motor->gpioEn = gpioEn;
	motor->pinEn = pinEn;
	motor->gpioDir = gpioDir;
	motor->pinDir = pinDir;


	// Disable PWM output
	HAL_TIM_PWM_Stop(motor->htim, motor->pwmCh);

	// Set Default Motor rotate direction
	HAL_GPIO_WritePin(motor->gpioDir, motor->pinDir, GPIO_PIN_RESET);

	// Disable power
	HAL_GPIO_WritePin(motor->gpioEn, motor->pinEn, GPIO_PIN_RESET);
	motor->status = DCMOTOR_DISABLED;
	motor->speed = 0;
	motor->dir = DCMOTOR_DIR_CW;

}


void DCMotor_EnablePower(DCMotor_t *motor)
{
	HAL_GPIO_WritePin(motor->gpioEn, motor->pinEn, GPIO_PIN_SET);
	if (motor->status == DCMOTOR_DISABLED) {
		motor->status = DCMOTOR_IDLE;
	}
}


void DCMotor_DisablePower(DCMotor_t *motor)
{
	HAL_TIM_PWM_Stop(motor->htim, motor->pwmCh);
	HAL_GPIO_WritePin(motor->gpioEn, motor->pinEn, GPIO_PIN_RESET);
	motor->status = DCMOTOR_DISABLED;
}

void DCMotor_SetDir(DCMotor_t *motor, DCMotor_Dir_t dir)
{
	if (motor->status == DCMOTOR_RUN_CW && dir == DCMOTOR_DIR_CCW) {
		HAL_TIM_PWM_Stop(motor->htim, motor->pwmCh);	// Stop first;
		HAL_GPIO_WritePin(motor->gpioDir, motor->pinDir, GPIO_PIN_SET);
		osDelay(10);

		HAL_TIM_PWM_Start(motor->htim, motor->pwmCh);
		motor->status = DCMOTOR_RUN_CCW;
	} else if (motor->status == DCMOTOR_RUN_CCW && dir == DCMOTOR_DIR_CW) {
		HAL_TIM_PWM_Stop(motor->htim, motor->pwmCh);	// Stop first;
		HAL_GPIO_WritePin(motor->gpioDir, motor->pinDir, GPIO_PIN_RESET);
		osDelay(10);
		HAL_TIM_PWM_Start(motor->htim, motor->pwmCh);
		motor->status = DCMOTOR_RUN_CW;
	} else {
		if (dir == DCMOTOR_DIR_CW) {
			HAL_GPIO_WritePin(motor->gpioDir, motor->pinDir, GPIO_PIN_RESET);
		} else {
			HAL_GPIO_WritePin(motor->gpioDir, motor->pinDir, GPIO_PIN_SET);
		}
	}
	motor->dir = dir;
}

void DCMotor_SetSpeed(DCMotor_t *motor, uint16_t  speed)
{
	if (speed > MAX_PWM_VAL) {
		speed = MAX_PWM_VAL;
	}

	__HAL_TIM_SET_COMPARE(motor->htim, motor->pwmCh, speed);
	motor->speed = speed;
}

void DCMotor_RunCW(DCMotor_t *motor, uint16_t speed)
{
	if (speed > MAX_PWM_VAL) {
		speed = MAX_PWM_VAL;
	}
	switch (motor->status) {
	case DCMOTOR_DISABLED:
		HAL_GPIO_WritePin(motor->gpioEn, motor->pinEn, GPIO_PIN_SET);
		HAL_GPIO_WritePin(motor->gpioDir, motor->pinDir, GPIO_PIN_RESET);
		__HAL_TIM_SET_COMPARE(motor->htim, motor->pwmCh, speed);
		HAL_TIM_PWM_Start(motor->htim, motor->pwmCh);
		break;

	case DCMOTOR_IDLE:
		HAL_GPIO_WritePin(motor->gpioDir, motor->pinDir, GPIO_PIN_RESET);
		__HAL_TIM_SET_COMPARE(motor->htim, motor->pwmCh, speed);
		HAL_TIM_PWM_Start(motor->htim, motor->pwmCh);
		break;

	case DCMOTOR_RUN_CW:
		__HAL_TIM_SET_COMPARE(motor->htim, motor->pwmCh, speed);
		break;

	case DCMOTOR_RUN_CCW:
		// Stop first
		HAL_TIM_PWM_Stop(motor->htim, motor->pwmCh);
		osDelay(10);
		// Set direction
		HAL_GPIO_WritePin(motor->gpioDir, motor->pinDir, GPIO_PIN_RESET);
		// Set speed
		__HAL_TIM_SET_COMPARE(motor->htim, motor->pwmCh, speed);
		// Start
		HAL_TIM_PWM_Start(motor->htim, motor->pwmCh);
		break;

	default:
		HAL_TIM_PWM_Stop(motor->htim, motor->pwmCh);
		HAL_GPIO_WritePin(motor->gpioEn, motor->pinEn, GPIO_PIN_RESET);
		break;
	}

	motor->status = DCMOTOR_RUN_CW;
	motor->dir = DCMOTOR_DIR_CW;
	motor->speed = speed;
}

void DCMotor_RunCCW(DCMotor_t *motor, uint16_t speed)
{
	if (speed > MAX_PWM_VAL) {
		speed = MAX_PWM_VAL;
	}
	switch (motor->status) {
	case DCMOTOR_DISABLED:
		HAL_GPIO_WritePin(motor->gpioEn, motor->pinEn, GPIO_PIN_SET);
		HAL_GPIO_WritePin(motor->gpioDir, motor->pinDir, GPIO_PIN_SET);
		__HAL_TIM_SET_COMPARE(motor->htim, motor->pwmCh, speed);
		HAL_TIM_PWM_Start(motor->htim, motor->pwmCh);
		break;

	case DCMOTOR_IDLE:
		HAL_GPIO_WritePin(motor->gpioDir, motor->pinDir, GPIO_PIN_SET);
		__HAL_TIM_SET_COMPARE(motor->htim, motor->pwmCh, speed);
		HAL_TIM_PWM_Start(motor->htim, motor->pwmCh);
		break;

	case DCMOTOR_RUN_CW:
		// Stop first
		HAL_TIM_PWM_Stop(motor->htim, motor->pwmCh);
		osDelay(10);
		// Set direction
		HAL_GPIO_WritePin(motor->gpioDir, motor->pinDir, GPIO_PIN_SET);
		// Set speed
		__HAL_TIM_SET_COMPARE(motor->htim, motor->pwmCh, speed);
		// Start
		HAL_TIM_PWM_Start(motor->htim, motor->pwmCh);
		break;

	case DCMOTOR_RUN_CCW:
		__HAL_TIM_SET_COMPARE(motor->htim, motor->pwmCh, speed);
		break;

	default:
		HAL_TIM_PWM_Stop(motor->htim, motor->pwmCh);
		HAL_GPIO_WritePin(motor->gpioEn, motor->pinEn, GPIO_PIN_RESET);
		break;
	}

	motor->status = DCMOTOR_RUN_CCW;
	motor->dir = DCMOTOR_DIR_CCW;
	motor->speed = speed;
}

DCMotor_Status_t DCMotor_GetStatus(DCMotor_t *motor)
{
	return motor->status;
}

DCMotor_Dir_t	DCMotor_GetDir(DCMotor_t *motor)
{
	return motor->dir;
}

uint16_t		DCMotor_GetSpeed(DCMotor_t *motor)
{
	return motor->speed;
}



