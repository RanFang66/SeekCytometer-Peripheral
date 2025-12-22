/*
 * seal_motor_control.c
 *
 *  Created on: 2025年12月2日
 *      Author: ranfa
 */

#include <seal_control.h>
#include "bsp_tim.h"
#include "bsp_gpio.h"
#include "bsp_adc.h"
#include "cmsis_os2.h"
#include "debug_shell.h"

static DCMotor_t 			sealMotor;
static SealCtrlCtx_t  		sealCtrlCtx;
static osMessageQueueId_t 	sealCmdQueue = NULL;
static osThreadId_t			sealThread = NULL;


void SealCtrl_Init()
{
	DCMotor_Init(&sealMotor, &SEAL_MOTOR_TIM, SEAL_MOTOR_PWM_CH, SEAL_MOTOR_EN_GPIO, SEAL_MOTOR_EN_PIN, SEAL_MOTOR_DIR_GPIO, SEAL_MOTOR_DIR_PIN);
	sealCtrlCtx.motor = &sealMotor;
	sealCtrlCtx.status = SEAL_IDLE;
	sealCtrlCtx.pushSpeed = DEFAULT_PUSH_SPEED;
	sealCtrlCtx.releaseSpeed = DEFAULT_RELEASE_SPEED;
	sealCtrlCtx.pushTimeLimit = DEFAULT_PUSH_TIME_LIMIT;
	sealCtrlCtx.releaseTimeLimit = DEFAULT_RELEASE_TIME_LIMIT;
	sealCtrlCtx.pushedCurrentThresh = DEFAULT_PUSH_CURRENT_THRESH;
	sealCtrlCtx.motorCurrentFaultThresh = DEFAULT_MOTOR_FAULT_THRESH;
}

uint8_t SealCtrl_SealReleased()
{
	GPIO_PinState st = HAL_GPIO_ReadPin(SEAL_PUSHED_GPIO, SEAL_PUSHED_PIN);
	return (uint8_t)st;
}

// Wrap DC motor operation to seal control operation
#define SealMotorStop() 	DCMotor_DisablePower(&sealMotor)
#define SealMotorPush() 	DCMotor_RunCCW(&sealMotor, sealCtrlCtx.pushSpeed);
#define SealMotorRelease() 	DCMotor_RunCW(&sealMotor, sealCtrlCtx.releaseSpeed)


static uint16_t maxI = 0;
uint16_t getMaxI()
{
	return maxI;
}
static void SealCtrl_Task(void *arg)
{
	SealCommand_t	cmd;
	uint8_t isPushed;
	uint16_t motorI;
	uint8_t isReleased;
	uint32_t currentTimeStamp;

	for (;;) {
		// Update Status
		isReleased = SealCtrl_SealReleased();
		motorI = GetMotorCurrentAdc();
		if (motorI > maxI) {
			maxI = motorI;
		}
		// Fault Check
		if (motorI > sealCtrlCtx.motorCurrentFaultThresh && sealCtrlCtx.status != SEAL_FAULT) {
			SealMotorStop();
			sealCtrlCtx.status = SEAL_FAULT;
//			LOG_ERROR("Seal Motor over current FAULT!");
		}


		isPushed = (motorI >= sealCtrlCtx.pushedCurrentThresh);
		currentTimeStamp = osKernelGetTickCount();
		// Handle position changed
		switch (sealCtrlCtx.status) {
		case SEAL_PUSHING:
			if (isPushed) {
				SealMotorStop();
				sealCtrlCtx.status = SEAL_IDLE;
			}
			if (sealCtrlCtx.pushStartTimestamp + sealCtrlCtx.pushTimeLimit < currentTimeStamp) {
				SealMotorStop();
				sealCtrlCtx.status = SEAL_FAULT;
//				LOG_ERROR("Seal push timeout!");

			}
			break;
		case SEAL_RELEASING:
			if (isReleased) {
				SealMotorStop();
				sealCtrlCtx.status = SEAL_IDLE;
			}
			if (currentTimeStamp > (sealCtrlCtx.releaseStartTime + sealCtrlCtx.releaseTimeLimit)) {
				SealMotorStop();
				sealCtrlCtx.status = SEAL_FAULT;
//				LOG_ERROR("Seal release timeout!");

			}
			break;
		default:
			break;
		}

		// Handle commands
		if (osMessageQueueGet(sealCmdQueue, &cmd, 0, 0) == osOK) {
			switch (cmd) {
			case SEAL_CMD_STOP:
				SealMotorStop();
				sealCtrlCtx.status = SEAL_IDLE;
				break;
			case SEAL_CMD_PUSH:
				maxI = 0;
				if (sealCtrlCtx.status != SEAL_FAULT && sealCtrlCtx.status != SEAL_PUSHING && !isPushed) {
					sealCtrlCtx.pushStartTimestamp = osKernelGetTickCount();
					SealMotorPush();
					sealCtrlCtx.status = SEAL_PUSHING;
				}
				break;
			case SEAL_CMD_RELEASE:
				maxI = 0;
				if (sealCtrlCtx.status != SEAL_FAULT && sealCtrlCtx.status != SEAL_RELEASING) {
					sealCtrlCtx.releaseStartTime = osKernelGetTickCount();
					SealMotorRelease();
					sealCtrlCtx.status = SEAL_RELEASING;
				}
				break;
			case SEAL_CMD_RESET:
				if (sealCtrlCtx.status == SEAL_FAULT) {
					SealMotorStop();
					maxI = 0;
					sealCtrlCtx.status = SEAL_IDLE;
				}
				break;
			default:
				LOG_WARNING("Undefined seal command!");
				break;
			}
		}
		osDelay(100);
	}
}


SealStatus_t SealCtrl_GetStatus()
{
	return sealCtrlCtx.status;
}

void sealCtrl_SetMotorFaultThresh(uint16_t thresh)
{
	sealCtrlCtx.motorCurrentFaultThresh = thresh;
}

void sealCtrl_SetMotorPushedThresh(uint16_t thresh)
{
	sealCtrlCtx.pushedCurrentThresh = thresh;
}

void SealCtrl_SetPushSpeed(uint16_t speed)
{
	sealCtrlCtx.pushSpeed = speed;
}


void SealCtrl_SetReleaseSpeed(uint16_t speed)
{
	sealCtrlCtx.releaseSpeed = speed;
}

void SealCtrl_SetPushTimeLimit(uint16_t timeout)
{
	sealCtrlCtx.pushTimeLimit = timeout;
}

void SealCtrl_SetReleaseTimeLimit(uint16_t timeout)
{
	sealCtrlCtx.releaseTimeLimit = timeout;
}

uint16_t SealCtrl_GetPushSpeed()
{
	return sealCtrlCtx.pushSpeed;
}
uint16_t SealCtrl_GetReleaseSpeed()
{
	return sealCtrlCtx.releaseSpeed;
}


uint16_t SealCtrl_GetPushTimeLimit()
{
	return sealCtrlCtx.pushTimeLimit;
}


uint16_t SealCtrl_GetReleaseTimeLimit()
{
	return sealCtrlCtx.releaseTimeLimit;
}


uint16_t SealCtrl_GetFaultCurrThresh()
{
	return sealCtrlCtx.motorCurrentFaultThresh;
}


uint16_t SealCtrl_GetPushedCurrThresh()
{
	return sealCtrlCtx.pushedCurrentThresh;
}


void SealCtrl_Push()
{
	SealCommand_t cmd = SEAL_CMD_PUSH;
	osStatus_t st = osMessageQueuePut(sealCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send seal push command FAILED!");
	}
}


void SealCtrl_Release()
{
	SealCommand_t cmd = SEAL_CMD_RELEASE;
	osStatus_t st = osMessageQueuePut(sealCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send seal release command FAILED!");
	}
}

void SealCtrl_Stop()
{
	SealCommand_t cmd = SEAL_CMD_STOP;
	osStatus_t st = osMessageQueuePut(sealCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send seal stop command FAILED!");
	}
}

void SealCtrl_Reset()
{
	SealCommand_t cmd = SEAL_CMD_RESET;
	osStatus_t st = osMessageQueuePut(sealCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send seal reset command FAILED!");
	}
}

void SealCtrl_StartTask()
{
	sealCmdQueue = osMessageQueueNew(SEAL_CMD_QUEUE_SIZE, sizeof(SealCommand_t), NULL);
	if (sealCmdQueue == NULL) {
		LOG_ERROR("Create seal command queue FAILED!");
	}

	const osThreadAttr_t taskAttr = {.name = "SealControl", .priority=osPriorityNormal, .stack_size=256};
	sealThread = osThreadNew(SealCtrl_Task, NULL, &taskAttr);

	if (sealThread == NULL) {
		LOG_ERROR("Create seal control thread FAILED!");
	} else {
		LOG_INFO("Create seal control thread OK");
	}

}
