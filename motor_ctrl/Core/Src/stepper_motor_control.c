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



/* --- Init-time Modbus helpers -------------------------------------------
 *
 * Every transaction below used to be fire-and-forget: the MB_Status_t went into
 * a local that nothing ever read. That is what made the X axis dead until it
 * had been homed once. main.c switches the 12 V rail on (Power_Enable12VPower())
 * immediately before the scheduler starts, and the drives need up to about a
 * second after that before they answer on the bus. SMotorCtrl_Init() starts
 * talking straight away, so the first address it touches -- X, 0x01 -- absorbed
 * the whole dead time and silently lost its configuration and its enable
 * sequence, while Y and Z, reached a couple of seconds later, came up on a live
 * bus. A manual HOME ends with InitMotorToPPMode(), so it re-ran the enable
 * sequence on a bus that was by then alive, which is why homing X once made it
 * work for the rest of the session.
 *
 * The fix is to stop assuming: wait for each drive to answer before configuring
 * it, retry every transaction, verify that the drive really reached PP mode and
 * Operation Enabled, and log whatever does not get through.
 */
#define SMOTOR_IO_RETRY			(5)		/* attempts per init transaction */
#define SMOTOR_MODE_RETRY		(3)		/* attempts for the whole PP/HM setup */
#define SMOTOR_BUS_GAP_MS		(2)		/* RTU inter-frame gap between init frames */
#define SMOTOR_RETRY_GAP_MS		(20)	/* back-off before retrying a lost frame */
#define SMOTOR_READY_TIMEOUT_MS	(4000)	/* how long a drive may take to boot */

/* Collision avoidance: X/Y may only traverse once the Z lens is clear of them.
 * The guard after the retract used to compare against 200000 -- ten times the
 * threshold -- so it matched every time and silently dropped the X/Y move. */
#define SMOTOR_Z_SAFE_POS		(20000)
#define SMOTOR_Z_RETRACT_POS	(22000)

static MB_Status_t readMotorReg(SMotorIndex_t id, uint16_t reg, uint16_t *val)
{
	MB_Status_t st = MB_ERROR_TIMEOUT;
	for (uint8_t i = 0; i < SMOTOR_IO_RETRY; i++) {
		st = MB_ReadHoldingRegs(CtrlCtx[id].motorAddr, reg, 1, val);
		if (st == MB_OK) {
			break;
		}
		osDelay(SMOTOR_RETRY_GAP_MS);
	}
	osDelay(SMOTOR_BUS_GAP_MS);
	if (st != MB_OK) {
		LOG_ERROR("Motor-%c read reg 0x%04X FAILED (%d)", CtrlCtx[id].name, reg, st);
	}
	return st;
}

static MB_Status_t writeMotorReg(SMotorIndex_t id, uint16_t reg, uint16_t val)
{
	MB_Status_t st = MB_ERROR_TIMEOUT;
	for (uint8_t i = 0; i < SMOTOR_IO_RETRY; i++) {
		st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, reg, val);
		if (st == MB_OK) {
			break;
		}
		osDelay(SMOTOR_RETRY_GAP_MS);
	}
	osDelay(SMOTOR_BUS_GAP_MS);
	if (st != MB_OK) {
		LOG_ERROR("Motor-%c write reg 0x%04X FAILED (%d)", CtrlCtx[id].name, reg, st);
	}
	return st;
}

static MB_Status_t writeMotorReg32(SMotorIndex_t id, uint16_t reg, uint32_t val)
{
	MB_Status_t st = MB_ERROR_TIMEOUT;
	for (uint8_t i = 0; i < SMOTOR_IO_RETRY; i++) {
		st = MB_Write32BitsWord(CtrlCtx[id].motorAddr, reg, val);
		if (st == MB_OK) {
			break;
		}
		osDelay(SMOTOR_RETRY_GAP_MS);
	}
	osDelay(SMOTOR_BUS_GAP_MS);
	if (st != MB_OK) {
		LOG_ERROR("Motor-%c write reg32 0x%04X FAILED (%d)", CtrlCtx[id].name, reg, st);
	}
	return st;
}

/*
 * Poll the drive's status word until it answers. Called before anything is
 * written to it, so that a drive which is still booting costs us a wait instead
 * of a lost configuration.
 */
static bool WaitDriveReady(SMotorIndex_t id)
{
	uint32_t start = osKernelGetTickCount();

	do {
		if (MB_ReadHoldingRegs(CtrlCtx[id].motorAddr, REG_STATUS_WORD, 1,
				&CtrlCtx[id].statusWord) == MB_OK) {
			return true;
		}
		osDelay(50);
	} while ((osKernelGetTickCount() - start) < SMOTOR_READY_TIMEOUT_MS);

	LOG_ERROR("Motor-%c (addr %u) did not answer within %u ms",
			CtrlCtx[id].name, CtrlCtx[id].motorAddr, SMOTOR_READY_TIMEOUT_MS);
	return false;
}

static void ClearFaultIfAny(SMotorIndex_t id)
{
	if (readMotorReg(id, REG_STATUS_WORD, &CtrlCtx[id].statusWord) != MB_OK) {
		return;
	}
	if (CtrlCtx[id].statusWord & SW_BIT_FAULT) {
		LOG_WARNING("Motor-%c in fault (SW 0x%04X), resetting", CtrlCtx[id].name, CtrlCtx[id].statusWord);
		writeMotorReg(id, REG_CTRL_WORD, CW_BIT_FAULT_RESET);
		/* Bit7 has to fall again before the drive leaves the reset state; the
		 * enable sequence below does that with its 0x06. */
		writeMotorReg(id, REG_CTRL_WORD, 0x0000);
	}
}

/*
 * Stepper Motor Enable Sequence:
 * Control Word: 0x06->0x07->0x0F
 *
 * Returns true only when the drive actually reports Operation Enabled -- an
 * axis that stops here will ignore every subsequent move command.
 * */
static bool StepperMotor_EnableSeq(SMotorIndex_t id)
{
	writeMotorReg(id, REG_CTRL_WORD, CW_VAL_0x06);
	writeMotorReg(id, REG_CTRL_WORD, CW_VAL_0x07);
	writeMotorReg(id, REG_CTRL_WORD, CW_VAL_0x0F);

	if (readMotorReg(id, REG_STATUS_WORD, &CtrlCtx[id].statusWord) != MB_OK) {
		return false;
	}
	return (CtrlCtx[id].statusWord & SW_BIT_OPERATION_ENABLED) != 0;
}


static void ConfigOneMotor(SMotorIndex_t id)
{
	writeMotorReg(id, REG_VCC_STD, DEFAULT_VCC_IN);

	writeMotorReg(id, REG_VCC_MIN, DEFAULT_VCC_MIN);

	if (id == MOTOR_X) {
		writeMotorReg(id, REG_DI1_FUNC, DI_FUNC_HOME);
		writeMotorReg(id, REG_DI1_LEVEL, DI_HIGH_VALID);
		writeMotorReg(id, REG_DI2_FUNC, DI_FUNC_UNUSED);
		writeMotorReg(id, REG_DI2_LEVEL, DI_HIGH_VALID);
		writeMotorReg(id, REG_POS_POLARITY, POS_POLARITY_POS);
	} else if (id == MOTOR_Y) {
		writeMotorReg(id, REG_DI1_FUNC, DI_FUNC_HOME);
		writeMotorReg(id, REG_DI1_LEVEL, DI_HIGH_VALID);
		writeMotorReg(id, REG_DI2_FUNC, DI_FUNC_POS_LIMIT);
		writeMotorReg(id, REG_DI2_LEVEL, DI_HIGH_VALID);
		writeMotorReg(id, REG_POS_POLARITY, POS_POLARITY_POS);
	} else if (id == MOTOR_Z) {
		writeMotorReg(id, REG_DI1_FUNC, DI_FUNC_HOME);
		writeMotorReg(id, REG_DI1_LEVEL, DI_HIGH_VALID);
		writeMotorReg(id, REG_DI2_FUNC, DI_FUNC_UNUSED);
		writeMotorReg(id, REG_DI2_LEVEL, DI_HIGH_VALID);
	}


	// Set position arrived judge threshold
	writeMotorReg32(id, REG_ARIVE_THRESH, DEFAULT_ARRIVE_THRESH);
}


static bool InitMotorToHomeMode(SMotorIndex_t id)
{
	CtrlCtx[id].ppReady = 0;	/* leaving PP mode */

	for (uint8_t attempt = 0; attempt < SMOTOR_MODE_RETRY; attempt++) {
		ClearFaultIfAny(id);

		// Configure to CiA402 Mode
		writeMotorReg(id, REG_CTRL_PROTOCOL, CTRL_MODE_CIA402);

		// Configure to HM Mode
		writeMotorReg(id, REG_MOTION_MODE, MOTION_MODE_HM);

		// Configure Home Mode Type
		writeMotorReg(id, REG_HOME_TYPE, CtrlCtx[id].homeModeType);

		// Configure Home Reset Time Limit
		writeMotorReg(id, REG_HOME_TIME_LIMIT, CtrlCtx[id].resetTimeLimit);

		// Configure Home Find zero acceleration speed
		writeMotorReg32(id, REG_HOME_ACC, 200000);

		// Configure Home Find speed
		writeMotorReg32(id, REG_HOME_HIGH_SPEED, 50000);

		// Configure Home Find zero speed
		writeMotorReg32(id, REG_HOME_LOW_SPEED, 8000);

		// Enable sequence
		if (!StepperMotor_EnableSeq(id)) {
			LOG_WARNING("Motor-%c not operation-enabled for HM mode (SW 0x%04X), retry %u",
					CtrlCtx[id].name, CtrlCtx[id].statusWord, attempt + 1);
			continue;
		}

		// Run
		if (writeMotorReg(id, REG_CTRL_WORD, 0x1F) == MB_OK) {
			return true;
		}
	}

	LOG_ERROR("Motor-%c failed to enter HM mode", CtrlCtx[id].name);
	return false;
}

static void StepperMotor_UpdateStatus(SMotorIndex_t id)
{
	MB_Status_t st;
	st = MB_ReadHoldingRegs(CtrlCtx[id].motorAddr, REG_STATUS_WORD, 1, &CtrlCtx[id].statusWord);
	st = MB_Read32BitsWord(CtrlCtx[id].motorAddr, REG_POS_FB, (uint32_t*)&CtrlCtx[id].motorPos);
	st = MB_ReadHoldingRegs(CtrlCtx[id].motorAddr, REG_DI_STAT, 1, &CtrlCtx[id].limitStatus);
	//st = MB_Read32BitsWord(CtrlCtx[id].motorAddr, REG_ERROR_CODE0, &CtrlCtx[id].errorCode);
	(void)st;
}


static bool InitMotorToPPMode(SMotorIndex_t id)
{
	for (uint8_t attempt = 0; attempt < SMOTOR_MODE_RETRY; attempt++) {
		ClearFaultIfAny(id);

		// Configure to CiA402 Mode
		writeMotorReg(id, REG_CTRL_PROTOCOL, CTRL_MODE_CIA402);

		// Configure to PP Mode
		writeMotorReg(id, REG_MOTION_MODE, MOTION_MODE_PP);

		// Configure stable speed
		writeMotorReg32(id, REG_VEL_SET, CtrlCtx[id].speed);

		// Configure acceleration speed
		writeMotorReg32(id, REG_ACC_SET, CtrlCtx[id].accSpeed);

		// Configure descent speed
		writeMotorReg32(id, REG_DESC_SET, CtrlCtx[id].descSpeed);

		if (!StepperMotor_EnableSeq(id)) {
			LOG_WARNING("Motor-%c not operation-enabled (SW 0x%04X), retry %u",
					CtrlCtx[id].name, CtrlCtx[id].statusWord, attempt + 1);
			continue;
		}

		/* Read the mode back. A drive that answers but stayed in another mode
		 * accepts the control word and then ignores the move, which is exactly
		 * the failure this whole function exists to make impossible. */
		uint16_t mode = 0;
		if (readMotorReg(id, REG_MOTION_MODE, &mode) == MB_OK && mode != MOTION_MODE_PP) {
			LOG_WARNING("Motor-%c mode reads back %u, expected PP, retry %u",
					CtrlCtx[id].name, mode, attempt + 1);
			continue;
		}

		LOG_INFO("Motor-%c ready in PP mode (SW 0x%04X)", CtrlCtx[id].name, CtrlCtx[id].statusWord);
		CtrlCtx[id].ppReady = 1;
		return true;
	}

	CtrlCtx[id].ppReady = 0;
	LOG_ERROR("Motor-%c failed to enter PP mode", CtrlCtx[id].name);
	return false;
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
	CtrlCtx[MOTOR_X].homeModeType = HOME_MODE_ORIGIN_20;

	CtrlCtx[MOTOR_Y].name = 'Y';
	CtrlCtx[MOTOR_Y].speed = 50000;
	CtrlCtx[MOTOR_Y].accSpeed = 100000;
	CtrlCtx[MOTOR_Y].descSpeed = 100000;
	CtrlCtx[MOTOR_Y].resetTimeLimit = 18000;
	CtrlCtx[MOTOR_Y].homeModeType = HOME_MODE_ORIGIN_21;

	CtrlCtx[MOTOR_Z].name = 'Z';
	CtrlCtx[MOTOR_Z].speed = 50000;
	CtrlCtx[MOTOR_Z].accSpeed = 100000;
	CtrlCtx[MOTOR_Z].descSpeed = 100000;
	CtrlCtx[MOTOR_Z].resetTimeLimit = 10000;
	CtrlCtx[MOTOR_Z].homeModeType = HOME_MODE_ORIGIN_19;

	/* One axis at a time, and only once that axis has proved it is listening.
	 * Configuring all three and only then enabling all three meant whichever
	 * drive was addressed first paid for the drives' power-up time. */
	for (SMotorIndex_t id = MOTOR_X; id <= MOTOR_Z; id++) {
		CtrlCtx[id].ppReady = 0;
		if (!WaitDriveReady(id)) {
			continue;	/* ensurePPMode() will retry on the first move command */
		}
		ConfigOneMotor(id);
		InitMotorToPPMode(id);
	}
}

/*
 * A move must never be issued to a drive that is not in PP mode and enabled --
 * it is accepted on the wire and then silently ignored. If the boot-time setup
 * did not get through (drive still powering up, cable, bus noise), redo it here
 * so the axis recovers by itself instead of staying dead until someone homes it.
 */
static bool ensurePPMode(SMotorIndex_t id)
{
	if (CtrlCtx[id].ppReady) {
		return true;
	}
	LOG_WARNING("Motor-%c not in PP mode, re-initialising before the move", CtrlCtx[id].name);
	return InitMotorToPPMode(id);
}

static void StepperMotor_RunSteps(SMotorIndex_t id, int32_t steps)
{
	MB_Status_t st;
	if (!ensurePPMode(id)) {
		return;
	}
	st = MB_Write32BitsWord(CtrlCtx[id].motorAddr, REG_POS_SET, steps);
	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_CTRL_WORD, CW_REL_WAIT);
	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_CTRL_WORD, CW_REL_WAIT | 0x0010);
	(void)st;
}


static void StepperMotor_RunToPos(SMotorIndex_t id, int32_t pos)
{
	MB_Status_t st;
	if (!ensurePPMode(id)) {
		return;
	}
	st = MB_Write32BitsWord(CtrlCtx[id].motorAddr, REG_POS_SET, pos);
	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_CTRL_WORD, CW_ABS_IMMEDIATE);
	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_CTRL_WORD, CW_ABS_IMMEDIATE | 0x0010);
	(void)st;
}

static void StepperMotor_Stop(SMotorIndex_t id)
{
	MB_Status_t st;
	st = MB_WriteSingleReg(CtrlCtx[id].motorAddr, REG_CTRL_WORD, CW_VAL_0x07);
	(void)st;
}

static void StepperMotor_FindHome(SMotorIndex_t id)
{
	if (!InitMotorToHomeMode(id)) {
		/* Nothing was started, so waiting 10 s for a home flag is pointless.
		 * Put the axis back in PP mode so ordinary moves can still be tried. */
		InitMotorToPPMode(id);
		return;
	}

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
			/* Not "break": that would leave the for(;;) loop and return from the
			 * task entry function, which FreeRTOS treats as fatal. */
			if (cmd.motorId > MOTOR_Z) {
				LOG_WARNING("Ignoring stepper command for invalid motor id %d", cmd.motorId);
				osDelay(200);
				continue;
			}
			switch (cmd.cmdType) {
			case STEPPER_MOTOR_STOP:
				StepperMotor_Stop(cmd.motorId);
				break;
			case STEPPER_MOTOR_RUN_STEPS:
				if (cmd.enCollisionAvoid  && (cmd.motorId == MOTOR_X || cmd.motorId == MOTOR_Y) && CtrlCtx[MOTOR_Z].motorPos < SMOTOR_Z_SAFE_POS) {
					uint16_t tCount = 0;
					StepperMotor_RunToPos(MOTOR_Z, SMOTOR_Z_RETRACT_POS);
					do {
						osDelay(500);
						tCount++;
						StepperMotor_UpdateStatus(MOTOR_Z);
					} while(tCount < 10 && !StepperMotor_ReachTarget(MOTOR_Z));

					if (CtrlCtx[MOTOR_Z].motorPos < SMOTOR_Z_SAFE_POS) {
						LOG_WARNING("Z still at %d, dropping motor-%c move",
								(int)CtrlCtx[MOTOR_Z].motorPos, CtrlCtx[cmd.motorId].name);
						osDelay(200);
						continue;
					}
				}

				StepperMotor_RunSteps(cmd.motorId, cmd.cmdData);
				break;
			case STEPPER_MOTOR_RUN_POS:
				if (cmd.enCollisionAvoid  && (cmd.motorId == MOTOR_X || cmd.motorId == MOTOR_Y) && CtrlCtx[MOTOR_Z].motorPos < SMOTOR_Z_SAFE_POS) {
					uint16_t tCount = 0;
					StepperMotor_RunToPos(MOTOR_Z, SMOTOR_Z_RETRACT_POS);
					do {
						osDelay(500);
						tCount++;
						StepperMotor_UpdateStatus(MOTOR_Z);
					} while(tCount < 10 && !StepperMotor_ReachTarget(MOTOR_Z));

					if (CtrlCtx[MOTOR_Z].motorPos < SMOTOR_Z_SAFE_POS) {
						LOG_WARNING("Z still at %d, dropping motor-%c move",
								(int)CtrlCtx[MOTOR_Z].motorPos, CtrlCtx[cmd.motorId].name);
						osDelay(200);
						continue;
					}
				}


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

	const osThreadAttr_t taskAttr = {.name = "StepperMotorControl", .priority=osPriorityNormal, .stack_size=768};
	SMotorCtrlThread = osThreadNew(SMotorCtrl_Task, NULL, &taskAttr);

	if (SMotorCtrlThread == NULL) {
		LOG_ERROR("Create stepper motor control thread FAILED!");
	} else {
		LOG_INFO("Create stepper motor control thread OK");
	}
}

void SMotorCtrl_RunToPos(SMotorIndex_t id, int32_t pos, uint8_t enCollisionAvoid)
{
	SMotorCmd_t cmd = {.cmdType = STEPPER_MOTOR_RUN_POS, .motorId = id, .cmdData = pos, .enCollisionAvoid = enCollisionAvoid};
	osStatus_t st = osMessageQueuePut(SMotorCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send stepper motor RUN TO POS command FAILED!");
	}
}
void SMotorCtrl_RunSteps(SMotorIndex_t id, int32_t steps, uint8_t enCollisionAvoid)
{
	SMotorCmd_t cmd = {.cmdType = STEPPER_MOTOR_RUN_STEPS, .motorId = id, .cmdData = steps, .enCollisionAvoid = enCollisionAvoid};
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

bool SMotorCtrl_IsReady(SMotorIndex_t id)
{
	return CtrlCtx[id].ppReady != 0;
}




