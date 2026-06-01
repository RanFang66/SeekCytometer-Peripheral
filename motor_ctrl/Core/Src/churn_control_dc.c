/*
 * churn_control_dc.c
 *
 *  Created on: 2026年6月1日
 *      Author: ranfa
 */

#include "churn_control_dc.h"
#include "bsp_tim.h"
#include "main.h"
#include "cmsis_os2.h"
#include "debug_shell.h"

static SwPwm_t				churnDcPwm;
static ChurnDcCtrlCtx_t		churnDcCtrlCtx;
static osThreadId_t			churnDcThread = NULL;
static osMessageQueueId_t	churnDcCmdQueue = NULL;

void ChurnDcCtrl_Init(void)
{
	SwPwm_Init(&churnDcPwm, churn_dc_motor_GPIO_Port, churn_dc_motor_Pin,
			&CHURN_DC_MOTOR_TIM, CHURN_DC_PWM_FREQ_HZ);

	churnDcCtrlCtx.pwm = &churnDcPwm;
	churnDcCtrlCtx.status = CHURN_DC_IDLE;
}

static void ChurnDcCtrl_Task(void *arg)
{
	(void)arg;
	ChurnDcCmd_t cmd;
	for (;;) {
		if (osMessageQueueGet(churnDcCmdQueue, &cmd, NULL, osWaitForever) == osOK) {
			switch (cmd.cmdType) {
			case CHURN_DC_CMD_STOP:
				SwPwm_Stop(&churnDcPwm);
				churnDcCtrlCtx.status = CHURN_DC_IDLE;
				break;

			case CHURN_DC_CMD_START:
				if (churnDcCtrlCtx.status != CHURN_DC_FAULT) {
					if (cmd.duty == 0U) {
						SwPwm_Stop(&churnDcPwm);
						churnDcCtrlCtx.status = CHURN_DC_IDLE;
					} else {
						SwPwm_Start(&churnDcPwm, cmd.duty);
						churnDcCtrlCtx.status = CHURN_DC_RUNNING;
					}
				}
				break;

			case CHURN_DC_CMD_SET_SPEED:
				if (churnDcCtrlCtx.status != CHURN_DC_FAULT) {
					SwPwm_SetDuty(&churnDcPwm, cmd.duty);
					churnDcCtrlCtx.status = (cmd.duty == 0U) ? CHURN_DC_IDLE : CHURN_DC_RUNNING;
				}
				break;

			case CHURN_DC_CMD_RESET:
				SwPwm_Stop(&churnDcPwm);
				churnDcCtrlCtx.status = CHURN_DC_IDLE;
				break;

			default:
				break;
			}
		}
	}
}

void ChurnDcCtrl_StartTask(void)
{
	churnDcCmdQueue = osMessageQueueNew(CHURN_DC_CMD_QUEUE_SIZE, sizeof(ChurnDcCmd_t), NULL);
	if (churnDcCmdQueue == NULL) {
		LOG_ERROR("Create churn DC command queue FAILED!");
	}

	const osThreadAttr_t taskAttr = {.name = "ChurnDcControl", .priority = osPriorityNormal, .stack_size = 256};
	churnDcThread = osThreadNew(ChurnDcCtrl_Task, NULL, &taskAttr);

	if (churnDcThread == NULL) {
		LOG_ERROR("Create churn DC control thread FAILED!");
	} else {
		LOG_INFO("Create churn DC control thread OK");
	}
}

static void ChurnDcCtrl_SendCmd(ChurnDcCmdType_t type, uint8_t duty, const char *errMsg)
{
	ChurnDcCmd_t cmd = {.cmdType = type, .duty = duty};
	osStatus_t st = osMessageQueuePut(churnDcCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("%s", errMsg);
	}
}

void ChurnDcCtrl_Start(uint8_t duty)
{
	ChurnDcCtrl_SendCmd(CHURN_DC_CMD_START, duty, "Send churn DC start command FAILED!");
}

void ChurnDcCtrl_SetSpeed(uint8_t duty)
{
	ChurnDcCtrl_SendCmd(CHURN_DC_CMD_SET_SPEED, duty, "Send churn DC set speed command FAILED!");
}

void ChurnDcCtrl_Stop(void)
{
	ChurnDcCtrl_SendCmd(CHURN_DC_CMD_STOP, 0, "Send churn DC stop command FAILED!");
}

void ChurnDcCtrl_Reset(void)
{
	ChurnDcCtrl_SendCmd(CHURN_DC_CMD_RESET, 0, "Send churn DC reset command FAILED!");
}

ChurnDcStatus_t ChurnDcCtrl_GetStatus(void)
{
	return churnDcCtrlCtx.status;
}

uint8_t ChurnDcCtrl_GetSpeed(void)
{
	return SwPwm_GetDuty(&churnDcPwm);
}
