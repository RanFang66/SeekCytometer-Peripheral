/*
 * temperature_control.h
 *
 *  Created on: 2025年12月3日
 *      Author: ranfa
 */

#ifndef INC_TEMPERATURE_CONTROL_H_
#define INC_TEMPERATURE_CONTROL_H_

#include "pid.h"
#include "tim.h"

typedef struct {
	TIM_HandleTypeDef 	*htim;
	uint32_t 			pwmChannel;
	uint8_t				status;
	uint16_t			pwmVal;
} Peltier_t;

typedef struct {
	TIM_HandleTypeDef 	*htim;
	uint32_t 			pwmChannel;
	uint8_t 			status;
	uint16_t			pwmVal;
} Fan_t;

#define PELTIER_NUM					(2)
#define COOL_FAN_NUM				(4)
#define TEMP_CTRL_CMD_QUEUE_SIZE 	(5)

typedef enum {
	TEMP_CTRL_IDLE = 0,
	TEMP_CTRL_RUNNING,
	TEMP_CTRL_FAULT
} TempCtrlStatus_t;

typedef enum {
	TEMP_CTRL_CMD_STOP = 0,
	TEMP_CTRL_CMD_START,
	TEMP_CTRL_SET_TARGET,
	TEMP_CTRL_FAN_SET,
	TEMP_CTRL_FAN_ENABLE,
	TEMP_CTRL_FAN_DISABLE,
	TEMP_CTRL_FAN_SET_SPEED,
	TEMP_CTRL_CMD_RESET,
} TempCmdType_t;

typedef struct {
	TempCmdType_t 	cmdType;
	float 			targetTemp;
	uint8_t			fanEn;
	uint16_t 		fanSpeed[COOL_FAN_NUM];
} TempCtrlCmd_t;

typedef struct {
	PID_HandleTypeDef 	*pid;
	Peltier_t			*peltier[PELTIER_NUM];
	Fan_t				*fan[COOL_FAN_NUM];

	float 				temp_min;
	float 				temp_max;
	float 				temp_target;
	float 				temp_latest;
	uint8_t 			peltier_enable[PELTIER_NUM];
//	uint8_t 			fan_enable[COOL_FAN_NUM];
//	uint16_t			fan_speed[COOL_FAN_NUM];
	TempCtrlStatus_t 	status;
} TempCtrlCtx_t;

#define DEFAULT_KP			1.0f
#define DEFAULT_KI			1.0f
#define DEFAULT_KD			0.0f

#define DEFAULT_TAU			0.0f
#define DEFAULT_OUT_MIN		0.0f
#define DEFAULT_OUT_MAX 	10000.0f
#define DEFAULT_STEP_MIN 	-1000.0f
#define DEFAULT_STEP_MAX	1000.0f

#define DEFAULT_TEMP_MIN	-10.0f
#define DEFAULT_TEMP_MAX	30.0f

#define DEFAULT_FAN_SPEED 	30000
#define DEFAULT_TEMP_TARGET	4.0f


void TempCtrl_Init();
void TempCtrl_SetTarget(float temp);
void TempCtrl_Start(float target);
void TempCtrl_FanSet(uint8_t fanEnCh, uint16_t fanSpeed[COOL_FAN_NUM]);
void TempCtrl_EnableFan(uint8_t fanCh);
void TempCtrl_DisableFan(uint8_t fanCh);
void TempCtrl_SetFanSpeed(uint8_t fanCh, uint16_t speed);

void TempCtrl_Stop();
void TempCtrl_Reset();
TempCtrlStatus_t TempCtrl_GetStatus();
float TempCtrl_GetTempTarget();
float TempCtrl_GetTempLatest();
float TempCtrl_GetKp();
float TempCtrl_GetKi();
uint8_t TempCtrl_GetFanStatus(uint8_t id);
uint16_t TempCtrl_GetFanSpeed(uint8_t id);
uint8_t TempCtrl_GetPeltierStatus(uint8_t id);
uint16_t TempCtrl_GetPeltierOutput(uint8_t id);

void TempCtrl_StartTask();

#endif /* INC_TEMPERATURE_CONTROL_H_ */
