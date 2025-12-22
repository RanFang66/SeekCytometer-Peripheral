/*
 * stepper_motor_control.h
 *
 *  Created on: 2025年12月4日
 *      Author: ranfa
 */

#ifndef INC_STEPPER_MOTOR_CONTROL_H_
#define INC_STEPPER_MOTOR_CONTROL_H_

#include "modbus_master.h"

#define STEPPER_MOTOR_NUM		(3)

typedef enum {
	MOTOR_X = 0,
	MOTOR_Y,
	MOTOR_Z,
	MOTOR_UNDEFINED,
} SMotorIndex_t;

#define MOTOR_X_ADDR		0x01
#define MOTOR_Y_ADDR		0x02
#define MOTOR_Z_ADDR		0X03

#define STEPPER_MOTOR_CMD_QUEUE_SIZE	16


#define DEFAULT_ARRIVE_THRESH	(3)
#define DEFAULT_VCC_IN			(11)
#define DEFAULT_VCC_MIN			(10)


typedef struct {
	char 		name;
	uint16_t 	motorAddr;
	uint32_t    speed;
	uint32_t 	accSpeed;
	uint32_t 	descSpeed;
	uint16_t 	resetTimeLimit;
	uint16_t 	homeModeType;
	uint16_t 	statusWord;
	int32_t  	motorPos;
	uint16_t	limitStatus;
	uint32_t 	errorCode;
} SMotorCtrlCtx_t;


typedef enum {
	STEPPER_MOTOR_STOP = 0,
	STEPPER_MOTOR_RUN_STEPS,
	STEPPER_MOTOR_RUN_POS,
	STEPPER_MOTOR_FIND_HOME,
	STEPPER_MOTOR_RESET,
} SMotorCmdType_t;

typedef struct {
	SMotorCmdType_t 	cmdType;
	SMotorIndex_t 		motorId;
	int32_t 			cmdData;
} SMotorCmd_t;

void SMotorCtrl_Init();
void SMotorCtrl_StartTask();
void SMotorCtrl_RunToPos(SMotorIndex_t id, int32_t pos);
void SMotorCtrl_RunSteps(SMotorIndex_t id, int32_t steps);
void SMotorCtrl_Stop(SMotorIndex_t id);
void SMotorCtrl_FindHome(SMotorIndex_t id);
void SMotorCtrl_Reset(SMotorIndex_t id);
char SMotorCtrl_GetName(SMotorIndex_t id);
uint16_t SMotorCtrl_GetStatus(SMotorIndex_t id);
int32_t SMotorCtrl_GetPos(SMotorIndex_t id);
uint16_t SMotorCtrl_GetLimitStatus(SMotorIndex_t id);

#endif /* INC_STEPPER_MOTOR_CONTROL_H_ */
