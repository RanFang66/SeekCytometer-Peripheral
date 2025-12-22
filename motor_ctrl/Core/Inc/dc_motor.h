/*
 * dc_motor.h
 *
 *  Created on: 2025年11月28日
 *      Author: ranfa
 */

#ifndef INC_DC_MOTOR_H_
#define INC_DC_MOTOR_H_

#include "tim.h"
#include "gpio.h"

typedef enum {
	DCMOTOR_DISABLED = 0,
	DCMOTOR_IDLE,
	DCMOTOR_RUN_CW,
	DCMOTOR_RUN_CCW,
	DCMOTOR_FAULT,
} DCMotor_Status_t;

typedef enum {
	DCMOTOR_DIR_CW = 0,
	DCMOTOR_DIR_CCW,
} DCMotor_Dir_t;

typedef struct {
	TIM_HandleTypeDef 	*htim;
	uint32_t 			pwmCh;
	GPIO_TypeDef		*gpioEn;
	uint16_t			pinEn;
	GPIO_TypeDef 		*gpioDir;
	uint16_t			pinDir;
	DCMotor_Status_t	status;
	DCMotor_Dir_t 		dir;
	uint16_t			speed;
} DCMotor_t;


void DCMotor_Init(DCMotor_t *motor, TIM_HandleTypeDef *htim, uint32_t ch, GPIO_TypeDef* gpioEn, uint16_t pinEn, GPIO_TypeDef *gpioDir, uint16_t pinDir);

void DCMotor_EnablePower(DCMotor_t *motor);
void DCMotor_DisablePower(DCMotor_t *motor);
void DCMotor_SetDir(DCMotor_t *motor, DCMotor_Dir_t dir);
void DCMotor_SetSpeed(DCMotor_t *motor, uint16_t  speed);
void DCMotor_RunCW(DCMotor_t *motor, uint16_t speed);
void DCMotor_RunCCW(DCMotor_t *motor, uint16_t speed);


DCMotor_Status_t DCMotor_GetStatus(DCMotor_t *motor);
DCMotor_Dir_t	DCMotor_GetDir(DCMotor_t *motor);
uint16_t		DCMotor_GetSpeed(DCMotor_t *motor);



#endif /* INC_DC_MOTOR_H_ */
