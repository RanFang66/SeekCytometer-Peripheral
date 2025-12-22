/*
 * press_control.h
 *
 *  Created on: 2025年12月9日
 *      Author: ranfa
 */

#ifndef INC_PRESS_CONTROL_H_
#define INC_PRESS_CONTROL_H_

#include "pid.h"



#define PRESS_CTRL_DEFAULT_KP		12.0f
#define PRESS_CTRL_DEFAULT_KI		8.0f
#define PRESS_CTRL_DEFAULT_KD		0.0f
#define PRESS_CTRL_DEFAULT_TAU		0.0f

#define PRESS_CTRL_DEFAULT_OUT_MIN			0
#define PRESS_CTRL_DEFAULT_OUT_MAX			50000
#define PRESS_CTRL_DEFAULT_STEP_MIN			-1000
#define PRESS_CTRL_DEFAULT_STEP_MAX 		1000
#define PRESS_CTRL_DEFAULT_FEEDFORWARD		14000

#define PRESS_CH_1		0x01
#define PRESS_CH_2		0x02
#define PRESS_CH_3		0x04
#define PRESS_CH_4		0x08
#define PRESS_CH_5		0x10
#define PRESS_CH_ALL	0x1F

#define PRESS_CTRL_CH_NUM			(5)
#define PRESS_CTRL_CMD_QUEUE_SIZE 	(5)

typedef enum {
	PRESS_CTRL_IDLE = 0,
	PRESS_CTRL_RUNNING,
	PRESS_CTRL_FAULT,
} PressCtrl_State_t;

typedef struct {
	PressCtrl_State_t	state;
	uint16_t			target;
	float 				latestPress;
	uint32_t			startTime;
	uint32_t			lastUpdateTime;
	uint32_t 			lastControlTime;
	PID_HandleTypeDef 	pid;
} PressCtrlCtx_t;


typedef enum {
	PRESS_CTRL_STOP = 0,
	PRESS_CTRL_START,
	PRESS_CTRL_SET_TARGET,
	PRESS_CTRL_RESET,
	PRESS_CTRL_SET_PI,
} PressCmdType_t;

typedef struct {
	PressCmdType_t 	cmdType;
	uint8_t 		chSet;
	uint16_t 		target[PRESS_CTRL_CH_NUM];
	float			kp;
	float 			ki;
	float			feedforward;
} PressCtrlCmd_t;


void PressCtrl_Init();
void PressCtrl_Start(uint8_t ch, uint16_t target[PRESS_CTRL_CH_NUM]);
void PressCtrl_SetTarget(uint8_t ch, uint16_t target[PRESS_CTRL_CH_NUM]);
void PressCtrl_SetPI(uint8_t ch, uint16_t kp_x100, uint16_t ki_x100, uint16_t ff);

void PressCtrl_Stop(uint8_t ch);
float PressCtrl_GetLatestPress(uint8_t ch);
float PressCtrl_GetInputPress();
PressCtrl_State_t PressCtrl_GetStatus(uint8_t ch);
uint16_t PressCtrl_GetTarget(uint8_t ch);
float PressCtrl_GetKp(uint8_t ch);
float PressCtrl_GetKi(uint8_t ch);
float PressCtrl_GetFF(uint8_t ch);
void PressCtrl_StartTask();

#endif /* INC_PRESS_CONTROL_H_ */
