/*
 * stepper_motor_control.c
 *
 *  Created on: 2025年12月4日
 *      Author: ranfa
 */

#include "stepper_motor_control.h"
#include "stepper_motor_protocol.h"
#include "debug_shell.h"
#include "cmsis_os2.h"
#include <stdbool.h>

static SMotorCtrlCtx_t CtrlCtx[STEPPER_MOTOR_NUM];
static osMessageQueueId_t SMotorCmdQueue = NULL;
static osThreadId_t SMotorCtrlThread = NULL;


static bool StepperMotor_ErrorOccur(SMotorIndex_t id)
{
	return (CtrlCtx[id].statusWord & (SW_BIT_FAULT | SW_BIT_WARNING));
}

static bool StepperMotor_ReachTarget(SMotorIndex_t id)
{
	return (CtrlCtx[id].statusWord & SW_BIT_REACHED);
}

static bool StepperMotor_ReachHome(SMotorIndex_t id)
{
	return (CtrlCtx[id].statusWord & SW_BIT_HOME);
}



/*
 * Stepper Motor Enable Sequence:
 * Control Word: 0x06->0x07->0x0F
 * */
void StepperMotor_EnableSeq(SMotorIndex_t id)
{
	MB_Status_t st;
	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_CTRL_WORD, CW_VAL_0x06);
	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_CTRL_WORD, CW_VAL_0x07);
	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_CTRL_WORD, CW_VAL_0x0F);
}


void ConfigOneMotor(SMotorIndex_t id)
{
	MB_Status_t st;

	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_VCC_STD, DEFAULT_VCC_IN);

	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_VCC_MIN, DEFAULT_VCC_MIN);

	if (id == MOTOR_X) {
		st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_DI1_FUNC, DI_FUNC_HOME);
		st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_DI1_LEVEL, DI_HIGH_VALID);
		st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_DI2_FUNC, DI_FUNC_UNUSED);
		st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_DI2_LEVEL, DI_HIGH_VALID);
	} else if (id == MOTOR_Y) {
		st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_DI1_FUNC, DI_FUNC_NEGA_LIMIT);
		st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_DI1_LEVEL, DI_HIGH_VALID);
		st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_DI2_FUNC, DI_FUNC_POS_LIMIT);
		st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_DI2_LEVEL, DI_HIGH_VALID);
//		st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_POS_POLARITY, POS_POLARITY_INVERT);
	} else if (id == MOTOR_Z) {
		st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_DI1_FUNC, DI_FUNC_HOME);
		st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_DI1_LEVEL, DI_HIGH_VALID);
		st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_DI2_FUNC, DI_FUNC_UNUSED);
		st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_DI2_LEVEL, DI_HIGH_VALID);
	}


	// Set position arrived judge threshold
	st = MB_Write32BitsWord(CtrlCtx[id].motorAddr, REG_ARIVE_THRESH, DEFAULT_ARRIVE_THRESH);
}


static void InitMotorToHomeMode(SMotorIndex_t id)
{
	MB_Status_t st;
	uint8_t retryCnt = 0;
	// Update Status
	do {
		st = MB_ReadHoldingRegs(CtrlCtx[id].motorAddr, REG_STATUS_WORD, 1, &CtrlCtx[id].statusWord);
		retryCnt++;
	} while (st != MB_OK || retryCnt > 3);

	if (st != MB_OK) {
		LOG_ERROR("Can not read status word for motor-%c", CtrlCtx[id].name);
		return;
	}

	if (CtrlCtx[id].statusWord & SW_BIT_FAULT) {
		st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_CTRL_WORD, CW_BIT_FAULT_RESET);
		if (st == MB_OK) {
			LOG_INFO("Reset Motor-%c fault OK", CtrlCtx[id].name);
		} else {
			LOG_ERROR("Reset Motor-%c fault FAILED", CtrlCtx[id].name);
		}
	}

	// Configure to CiA402 Mode
	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_CTRL_PROTOCOL, CTRL_MODE_CIA402);

	// Configure to HM Mode
	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_MOTION_MODE, MOTION_MODE_HM);

	// Configure Home Mode Type
	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_HOME_TYPE, CtrlCtx[id].homeModeType);

	// Configure Home Reset Time Limit
	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_HOME_TIME_LIMIT, CtrlCtx[id].resetTimeLimit);

	// Configure Home Find zero acceleration speed
	st = MB_Write32BitsWord(CtrlCtx[id].motorAddr, REG_HOME_ACC, 200000);

	// Configure Home Find speed
	st = MB_Write32BitsWord(CtrlCtx[id].motorAddr, REG_HOME_HIGH_SPEED, 50000);

	// Configure Home Find zero speed
	st = MB_Write32BitsWord(CtrlCtx[id].motorAddr, REG_HOME_LOW_SPEED, 8000);

	// Enable sequence
	StepperMotor_EnableSeq(id);

	// Run
	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_CTRL_WORD, 0x1F);

}

static void StepperMotor_UpdateStatus(SMotorIndex_t id)
{
	MB_Status_t st;
	st = MB_ReadHoldingRegs(CtrlCtx[id].motorAddr, REG_STATUS_WORD, 1, &CtrlCtx[id].statusWord);
	st = MB_Read32BitsWord(CtrlCtx[id].motorAddr, REG_POS_FB, (uint32_t*)&CtrlCtx[id].motorPos);
	st = MB_ReadHoldingRegs(CtrlCtx[id].motorAddr, REG_DI_STAT, 1, &CtrlCtx[id].limitStatus);
	//st = MB_Read32BitsWord(CtrlCtx[id].motorAddr, REG_ERROR_CODE0, &CtrlCtx[id].errorCode);
}


static void InitMotorToPPMode(SMotorIndex_t id)
{
	MB_Status_t st;
	uint8_t retryCnt = 0;
//	// Update Status
//	do {
//		st = MB_ReadHoldingRegs(CtrlCtx[id].motorAddr, REG_STATUS_WORD, 1, &CtrlCtx[id].statusWord);
//		retryCnt++;
//	} while (st != MB_OK || retryCnt > 3);
//
//	if (st != MB_OK) {
//		LOG_ERROR("Can not read status word for motor-%c", CtrlCtx[id].name);
//		return;
//	}
//
//	if (CtrlCtx[id].statusWord & SW_BIT_FAULT) {
//		st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_CTRL_WORD, CW_BIT_FAULT_RESET);
//		if (st == MB_OK) {
//			LOG_INFO("Reset Motor-%c fault OK", CtrlCtx[id].name);
//		} else {
//			LOG_ERROR("Reset Motor-%c fault FAILED", CtrlCtx[id].name);
//		}
//	}
	StepperMotor_UpdateStatus(id);
	if (CtrlCtx[id].statusWord & SW_BIT_FAULT) {
		st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_CTRL_WORD, CW_BIT_FAULT_RESET);
	}

	// Configure to CiA402 Mode
	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_CTRL_PROTOCOL, CTRL_MODE_CIA402);

	// Configure to PP Mode
	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_MOTION_MODE, MOTION_MODE_PP);

	// Configure stable speed
	st = MB_Write32BitsWord(CtrlCtx[id].motorAddr, REG_VEL_SET, CtrlCtx[id].speed);

	// Configure acceleration speed
	st = MB_Write32BitsWord(CtrlCtx[id].motorAddr, REG_ACC_SET, CtrlCtx[id].accSpeed);

	// Configure descent speed
	st = MB_Write32BitsWord(CtrlCtx[id].motorAddr, REG_DESC_SET, CtrlCtx[id].descSpeed);


	StepperMotor_EnableSeq(id);
}

void SMotorCtrl_Init()
{
	CtrlCtx[MOTOR_X].motorAddr = MOTOR_X_ADDR;
	CtrlCtx[MOTOR_Y].motorAddr = MOTOR_Y_ADDR;
	CtrlCtx[MOTOR_Z].motorAddr = MOTOR_Z_ADDR;

	CtrlCtx[MOTOR_X].name = 'X';
	CtrlCtx[MOTOR_X].speed = 50000;
	CtrlCtx[MOTOR_X].accSpeed = 100000;
	CtrlCtx[MOTOR_X].descSpeed = 100000;
	CtrlCtx[MOTOR_X].resetTimeLimit = 10000;
	CtrlCtx[MOTOR_X].homeModeType = HOME_MODE_ORIGIN_21;

	CtrlCtx[MOTOR_Y].name = 'Y';
	CtrlCtx[MOTOR_Y].speed = 50000;
	CtrlCtx[MOTOR_Y].accSpeed = 100000;
	CtrlCtx[MOTOR_Y].descSpeed = 100000;
	CtrlCtx[MOTOR_Y].resetTimeLimit = 10000;
	CtrlCtx[MOTOR_Y].homeModeType = HOME_MODE_NEGA_LIMIT;

	CtrlCtx[MOTOR_Z].name = 'Z';
	CtrlCtx[MOTOR_Z].speed = 50000;
	CtrlCtx[MOTOR_Z].accSpeed = 100000;
	CtrlCtx[MOTOR_Z].descSpeed = 100000;
	CtrlCtx[MOTOR_Z].resetTimeLimit = 10000;
	CtrlCtx[MOTOR_Z].homeModeType = HOME_MODE_ORIGIN_19;

	ConfigOneMotor(MOTOR_X);
	ConfigOneMotor(MOTOR_Y);
	ConfigOneMotor(MOTOR_Z);


	InitMotorToPPMode(MOTOR_X);
	InitMotorToPPMode(MOTOR_Y);
	InitMotorToPPMode(MOTOR_Z);
}

static void StepperMotor_RunSteps(SMotorIndex_t id, int32_t steps)
{
	MB_Status_t st;
	st = MB_Write32BitsWord(CtrlCtx[id].motorAddr, REG_POS_SET, steps);
	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_CTRL_WORD, CW_REL_WAIT);
	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_CTRL_WORD, CW_REL_WAIT | 0x0010);
}


static void StepperMotor_RunToPos(SMotorIndex_t id, int32_t pos)
{
	MB_Status_t st;
	st = MB_Write32BitsWord(CtrlCtx[id].motorAddr, REG_POS_SET, pos);
	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_CTRL_WORD, CW_ABS_IMMEDIATE);
	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_CTRL_WORD, CW_ABS_IMMEDIATE | 0x0010);
}

static void StepperMotor_Stop(SMotorIndex_t id)
{
	MB_Status_t st;
	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_CTRL_WORD, CW_VAL_0x07);
}

static void StepperMotor_FindHome(SMotorIndex_t id)
{
	InitMotorToHomeMode(id);

	uint16_t tcount = 0;
	do {
		osDelay(500);
		tcount++;
		StepperMotor_UpdateStatus(id);
	} while (tcount < 20 && !StepperMotor_ReachHome(id));
	InitMotorToPPMode(id);
}

static void StepperMotor_Reset(SMotorIndex_t id)
{
	InitMotorToPPMode(id);
}




static void SMotorCtrl_Task(void *arg)
{
	SMotorCmd_t cmd;
	osStatus_t ret;
	SMotorCtrl_Init();
	uint8_t count = 0;
	for (;;) {
		ret = osMessageQueueGet(SMotorCmdQueue, &cmd, NULL, 0);
		if (ret == osOK) {
			if (cmd.motorId < MOTOR_X ||cmd.motorId > MOTOR_Z) {
				break;
			}
			switch (cmd.cmdType) {
			case STEPPER_MOTOR_STOP:
				StepperMotor_Stop(cmd.motorId);
				break;
			case STEPPER_MOTOR_RUN_STEPS:
				if ((cmd.motorId == MOTOR_X || cmd.motorId == MOTOR_Y) && CtrlCtx[MOTOR_Z].motorPos < 20000) {
					uint16_t tCount = 0;
					StepperMotor_RunToPos(MOTOR_Z, 22000);
					do {
						osDelay(400);
						tCount++;
						StepperMotor_UpdateStatus(MOTOR_Z);
					} while(tCount < 10 && !StepperMotor_ReachTarget(MOTOR_Z));

					if (CtrlCtx[MOTOR_Z].motorPos < 200000) {
						continue;
					}
				}

				StepperMotor_RunSteps(cmd.motorId, cmd.cmdData);
				break;
			case STEPPER_MOTOR_RUN_POS:
				StepperMotor_RunToPos(cmd.motorId, cmd.cmdData);
				break;
			case STEPPER_MOTOR_FIND_HOME:
				StepperMotor_FindHome(cmd.motorId);
				break;
			case STEPPER_MOTOR_RESET:
				StepperMotor_Reset(cmd.motorId);
				break;
			default:
				break;
			}
		} else {
			if (count == 0) {
				StepperMotor_UpdateStatus(MOTOR_X);
				count = 1;
			} else if (count == 1) {
				StepperMotor_UpdateStatus(MOTOR_Y);
				count = 2;
			} else if (count == 2) {
				StepperMotor_UpdateStatus(MOTOR_Z);
				count = 0;
			}
		}
		osDelay(200);
	}
}


void SMotorCtrl_StartTask()
{
	SMotorCmdQueue = osMessageQueueNew(STEPPER_MOTOR_CMD_QUEUE_SIZE, sizeof(SMotorCmd_t), NULL);
	if (SMotorCmdQueue == NULL) {
		LOG_ERROR("Create stepper motor control command queue FAILED!");
	}

	const osThreadAttr_t taskAttr = {.name = "StepperMotorControl", .priority=osPriorityNormal, .stack_size=512};
	SMotorCtrlThread = osThreadNew(SMotorCtrl_Task, NULL, &taskAttr);

	if (SMotorCtrlThread == NULL) {
		LOG_ERROR("Create stepper motor control thread FAILED!");
	} else {
		LOG_INFO("Create stepper motor control thread OK");
	}
}

void SMotorCtrl_RunToPos(SMotorIndex_t id, int32_t pos)
{
	SMotorCmd_t cmd = {.cmdType = STEPPER_MOTOR_RUN_POS, .motorId = id, .cmdData = pos};
	osStatus_t st = osMessageQueuePut(SMotorCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send stepper motor RUN TO POS command FAILED!");
	}
}
void SMotorCtrl_RunSteps(SMotorIndex_t id, int32_t steps)
{
	SMotorCmd_t cmd = {.cmdType = STEPPER_MOTOR_RUN_STEPS, .motorId = id, .cmdData = steps};
	osStatus_t st = osMessageQueuePut(SMotorCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send stepper motor RUN STEPS command FAILED!");
	}
}
void SMotorCtrl_Stop(SMotorIndex_t id)
{
	SMotorCmd_t cmd = {.cmdType = STEPPER_MOTOR_STOP, .motorId = id, .cmdData = 0};
	osStatus_t st = osMessageQueuePut(SMotorCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send stepper motor STOP command  FAILED!");
	}
}

void SMotorCtrl_FindHome(SMotorIndex_t id)
{
	SMotorCmd_t cmd = {.cmdType = STEPPER_MOTOR_FIND_HOME, .motorId = id, .cmdData = 0};
	osStatus_t st = osMessageQueuePut(SMotorCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send stepper motor FIND HOME command  FAILED!");
	}
}
void SMotorCtrl_Reset(SMotorIndex_t id)
{
	SMotorCmd_t cmd = {.cmdType = STEPPER_MOTOR_RESET, .motorId = id, .cmdData = 0};
	osStatus_t st = osMessageQueuePut(SMotorCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send stepper motor RESET command  FAILED!");
	}
}

char SMotorCtrl_GetName(SMotorIndex_t id)
{
	return CtrlCtx[id].name;
}

uint16_t SMotorCtrl_GetStatus(SMotorIndex_t id)
{
	return CtrlCtx[id].statusWord;
}

int32_t SMotorCtrl_GetPos(SMotorIndex_t id)
{
	return CtrlCtx[id].motorPos;
}

uint16_t SMotorCtrl_GetLimitStatus(SMotorIndex_t id)
{
	return CtrlCtx[id].limitStatus;
}




