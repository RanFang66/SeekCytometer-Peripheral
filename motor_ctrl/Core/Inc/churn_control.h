/*
 * churn_control.h
 *
 *  Created on: 2025年12月2日
 *      Author: ranfa
 */

#ifndef INC_CHURN_CONTROL_H_
#define INC_CHURN_CONTROL_H_

#include "hr4988.h"

typedef enum {
	CHURN_IDLE = 0,
	CHURN_RUNNING_CW,
	CHURN_RUNNING_CCW,
	CHURN_FAULT,
} ChurnStatus_t;

#define DEFAULT_CHURN_SPEED_CW  (2 * 320)
#define DEFAULT_CHURN_SPEED_CCW (2 * 320)
#define CHURN_CMD_QUEUE_SIZE	(8)
typedef struct {
	HR4988_TypeDef 	*motor;
	ChurnStatus_t 	status;
} ChurnCtrlCtx_t;

typedef enum {
	CHURN_CMD_STOP = 0,
	CHURN_CMD_RUN_CW,
	CHURN_CMD_RUN_CCW,
	CHURN_CMD_RESET,
} ChurnCmdType_t;


typedef struct {
	ChurnCmdType_t 	cmdType;
	uint16_t 		speed;
} ChurnCmd_t;

void ChurnCtrl_Init();
void ChurnCtrl_StartTask();
void ChurnCtrl_Stop();
void ChurnCtrl_RunCW(uint16_t speed);
void ChurnCtrl_RunCCW(uint16_t speed);
void ChurnCtrl_Reset();
ChurnStatus_t ChurnCtrl_GetStatus();
uint16_t ChurnCtrl_GetSpeed();


#endif /* INC_CHURN_CONTROL_H_ */
