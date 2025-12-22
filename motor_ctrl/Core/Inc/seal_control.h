/*
 * seal_motor_control.h
 *
 *  Created on: 2025年12月2日
 *      Author: ranfa
 */

#ifndef INC_SEAL_CONTROL_H_
#define INC_SEAL_CONTROL_H_

#include "dc_motor.h"
#include "adc.h"

typedef enum {
	SEAL_IDLE = 0,
	SEAL_PUSHING,
	SEAL_RELEASING,
	SEAL_FAULT,
} SealStatus_t;

typedef enum {
	SEAL_CMD_STOP = 0,
	SEAL_CMD_PUSH,
	SEAL_CMD_RELEASE,
	SEAL_CMD_RESET,
} SealCommand_t;


typedef struct {
	DCMotor_t 			*motor;

	SealStatus_t 		status;
	uint16_t 			pushSpeed;
	uint16_t			releaseSpeed;
	uint16_t			motorCurrentFaultThresh;
	uint16_t			pushedCurrentThresh;
	uint16_t			pushTimeLimit;
	uint16_t			releaseTimeLimit;
	uint32_t			pushStartTimestamp;
	uint32_t			releaseStartTime;
} SealCtrlCtx_t;

#define SEAL_CMD_QUEUE_SIZE				(8)



/*Experiment results:
 * Released to sensor position, the max motor current is 70~80
 * When motor is stalling to push, the motor stall current is 400~500
 * So the suggested pushed motor current threshold value is 150~350
 * */
#define DEFAULT_PUSH_SPEED				(4000)
#define DEFAULT_RELEASE_SPEED 			(4000)
#define DEFAULT_PUSH_TIME_LIMIT			(3000)
#define DEFAULT_RELEASE_TIME_LIMIT		(3000)
#define DEFAULT_PUSH_CURRENT_THRESH		(300)
#define DEFAULT_MOTOR_FAULT_THRESH		(500)

void SealCtrl_Init();
SealStatus_t SealCtrl_GetStatus();
uint8_t SealCtrl_SealReleased();
void sealCtrl_SetMotorFaultThresh(uint16_t thresh);
void sealCtrl_SetMotorPushedThresh(uint16_t thresh);
void SealCtrl_SetPushSpeed(uint16_t speed);
void SealCtrl_SetReleaseSpeed(uint16_t speed);
void SealCtrl_SetPushTimeLimit(uint16_t timeout);
void SealCtrl_SetReleaseTimeLimit(uint16_t timeout);
uint16_t SealCtrl_GetPushSpeed();
uint16_t SealCtrl_GetReleaseSpeed();
uint16_t SealCtrl_GetPushTimeLimit();
uint16_t SealCtrl_GetReleaseTimeLimit();
uint16_t SealCtrl_GetFaultCurrThresh();
uint16_t SealCtrl_GetPushedCurrThresh();
void SealCtrl_Push();
void SealCtrl_Release();
void SealCtrl_Stop();
void SealCtrl_Reset();
void SealCtrl_StartTask();



#endif /* INC_SEAL_CONTROL_H_ */
