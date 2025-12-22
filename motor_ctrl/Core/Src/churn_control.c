/*
 * churn_control.c
 *
 *  Created on: 2025年12月2日
 *      Author: ranfa
 */

#include "churn_control.h"
#include "bsp_tim.h"
#include "bsp_gpio.h"
#include "cmsis_os2.h"
#include "debug_shell.h"

static HR4988_TypeDef 		churnMotor;
static ChurnCtrlCtx_t 		churnCtrlCtx;
static osThreadId_t			churnThread = NULL;
static osMessageQueueId_t	churnCmdQueue = NULL;


void ChurnCtrl_Init()
{
	HR4988_Init(&churnMotor, CHURN_MOTOR_EN_GPIO, CHURN_MOTOR_EN_PIN,
			CHURN_MOTOR_DIR_GPIO, CHURN_MOTOR_DIR_PIN, &CHURN_MOTOR_TIM, CHURN_MOTOR_PWM_CH);

	churnCtrlCtx.motor = &churnMotor;
	churnCtrlCtx.status = CHURN_IDLE;
}

#define ChurnMotorStop() 			HR4988_Disable(&churnMotor)
#define ChurnMotorRunCW(speed) 		HR4988_RunCW(&churnMotor, speed)
#define ChurnMotorRunCCW(speed) 	HR4988_RunCCW(&churnMotor, speed)


static void ChurnCtrl_Task(void *arg)
{
	ChurnCmd_t cmd;
	for (;;) {
		if (osMessageQueueGet(churnCmdQueue, &cmd, NULL, osWaitForever) == osOK) {
			switch (cmd.cmdType) {
			case CHURN_CMD_STOP:
				ChurnMotorStop();
				churnCtrlCtx.status = CHURN_IDLE;
				break;

			case CHURN_CMD_RUN_CW:
				if (churnCtrlCtx.status != CHURN_FAULT) {
					ChurnMotorRunCW(cmd.speed);
					churnCtrlCtx.status = CHURN_RUNNING_CW;
				}
				break;

			case CHURN_CMD_RUN_CCW:
				if (churnCtrlCtx.status != CHURN_FAULT) {
					ChurnMotorRunCCW(cmd.speed);
					churnCtrlCtx.status = CHURN_RUNNING_CCW;
				}
				break;

			case CHURN_CMD_RESET:
				ChurnMotorStop();
				churnCtrlCtx.status = CHURN_IDLE;
				break;

			default:
				break;
			}
		}
	}
}

void ChurnCtrl_StartTask()
{
	churnCmdQueue = osMessageQueueNew(CHURN_CMD_QUEUE_SIZE, sizeof(ChurnCmd_t), NULL);
	if (churnCmdQueue == NULL) {
		LOG_ERROR("Create churn command queue FAILED!");
	}

	const osThreadAttr_t taskAttr = {.name = "ChurnControl", .priority=osPriorityNormal, .stack_size=256};
	churnThread = osThreadNew(ChurnCtrl_Task, NULL, &taskAttr);

	if (churnThread == NULL) {
		LOG_ERROR("Create churn control thread FAILED!");
	} else {
		LOG_INFO("Create churn control thread OK");
	}
}

void ChurnCtrl_Stop()
{
	ChurnCmd_t cmd ={.cmdType = CHURN_CMD_STOP, .speed = 0};
	osStatus_t st = osMessageQueuePut(churnCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send churn stop command FAILED!");
	}
}

void ChurnCtrl_RunCW(uint16_t speed)
{
	ChurnCmd_t cmd = {.cmdType = CHURN_CMD_RUN_CW, .speed = speed};
	osStatus_t st = osMessageQueuePut(churnCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send churn run clockwise command FAILED!");
	}
}

void ChurnCtrl_RunCCW(uint16_t speed)
{
	ChurnCmd_t cmd = {.cmdType = CHURN_CMD_RUN_CCW, .speed = speed};
	osStatus_t st = osMessageQueuePut(churnCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send churn run counter-clockwise command FAILED!");
	}
}

void ChurnCtrl_Reset()
{
	ChurnCmd_t cmd = {.cmdType = CHURN_CMD_RESET, .speed = 0};
	osStatus_t st = osMessageQueuePut(churnCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send churn reset command FAILED!");
	}
}


ChurnStatus_t ChurnCtrl_GetStatus()
{
	return churnCtrlCtx.status;
}

uint16_t ChurnCtrl_GetSpeed()
{
	return churnMotor.freq;
}



