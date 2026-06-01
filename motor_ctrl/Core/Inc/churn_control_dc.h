/*
 * churn_control_dc.h
 *
 *  Created on: 2026年6月1日
 *      Author: ranfa
 *
 *  Control layer for the second churn motor: a DC motor driven by a single
 *  GPIO (PD11 / churn_dc_motor) through a software PWM. Speed is controlled by
 *  the PWM duty cycle (0..100 %); there is no direction control.
 */

#ifndef INC_CHURN_CONTROL_DC_H_
#define INC_CHURN_CONTROL_DC_H_

#include "swpwm.h"

typedef enum {
	CHURN_DC_IDLE = 0,
	CHURN_DC_RUNNING,
	CHURN_DC_FAULT,
} ChurnDcStatus_t;

#define CHURN_DC_PWM_FREQ_HZ		(5000U)		/* PWM carrier frequency */
#define DEFAULT_CHURN_DC_DUTY		(50U)		/* default speed in percent */
#define CHURN_DC_CMD_QUEUE_SIZE		(8)

typedef struct {
	SwPwm_t			*pwm;
	ChurnDcStatus_t	status;
} ChurnDcCtrlCtx_t;

typedef enum {
	CHURN_DC_CMD_STOP = 0,
	CHURN_DC_CMD_START,			/* start / set speed (duty) */
	CHURN_DC_CMD_SET_SPEED,
	CHURN_DC_CMD_RESET,
} ChurnDcCmdType_t;

typedef struct {
	ChurnDcCmdType_t	cmdType;
	uint8_t				duty;	/* 0..100 % */
} ChurnDcCmd_t;

void ChurnDcCtrl_Init(void);
void ChurnDcCtrl_StartTask(void);
void ChurnDcCtrl_Start(uint8_t duty);
void ChurnDcCtrl_SetSpeed(uint8_t duty);
void ChurnDcCtrl_Stop(void);
void ChurnDcCtrl_Reset(void);
ChurnDcStatus_t ChurnDcCtrl_GetStatus(void);
uint8_t ChurnDcCtrl_GetSpeed(void);

#endif /* INC_CHURN_CONTROL_DC_H_ */
