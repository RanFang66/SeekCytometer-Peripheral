/*
 * press_control.c
 *
 *  Created on: 2025年12月9日
 *      Author: ranfa
 */

#include "press_control.h"
#include <stdbool.h>
#include "cmsis_os2.h"
#include "debug_shell.h"
#include "propo_valve_drive.h"
#include "sol_valve_control.h"
#include "hsc_spi.h"
#include "hsc_conv.h"

static PressCtrlCtx_t pressCtrl[PRESS_CTRL_CH_NUM];
static osThreadId_t pressCtrlThread = NULL;
static osMessageQueueId_t pressCtrlCmdQueue = NULL;
static float inputPress = 0.0;



//#define IS_CH_ENABLE(ch, input)	(input & (0x01 << ch))


static inline bool isChEnable(uint8_t ch, uint8_t chSet)
{
	return (chSet & ((uint8_t)0x01 << ch)) != 0;
}

void PressCtrl_Init()
{
	for (uint8_t i = 0; i < PRESS_CTRL_CH_NUM; i++) {
		PID_Init(&pressCtrl[i].pid, PRESS_CTRL_DEFAULT_KP, PRESS_CTRL_DEFAULT_KI, PRESS_CTRL_DEFAULT_KD,
				PRESS_CTRL_DEFAULT_TAU,
				PRESS_CTRL_DEFAULT_OUT_MIN, PRESS_CTRL_DEFAULT_OUT_MAX,
				PRESS_CTRL_DEFAULT_STEP_MIN, PRESS_CTRL_DEFAULT_STEP_MAX,
				PRESS_CTRL_DEFAULT_FEEDFORWARD);
		pressCtrl[i].state = PRESS_CTRL_IDLE;
		pressCtrl[i].target = 0;
	}
}



static void stopPressCtrl(uint8_t id)
{
	// Close proportional valve
	PropoValveDrive_Close(id);

	// Close solenoid valve
	SOL_Close(id);

	// Reset PID
	PID_Reset(&pressCtrl[id].pid);
}

static void startPressCtrl(uint8_t id, uint16_t target)
{
	pressCtrl[id].startTime = osKernelGetTickCount();

	SOL_Open(id);

	pressCtrl[id].target = target;
}


static void changePressCtrlTarget(uint8_t id, uint16_t target)
{
	if (pressCtrl[id].state == PRESS_CTRL_RUNNING) {
		pressCtrl[id].startTime = osKernelGetTickCount();
	}
	pressCtrl[id].target = target;
}


static void resetPressCtrl(uint8_t id)
{
	// Close proportional valve
	PropoValveDrive_Close(id);

	// Close solenoid valve
	SOL_Close(id);

	// Reset PID
	PID_Reset(&pressCtrl[id].pid);
}


static void updateMeasuredPress()
{
	uint16_t pressure_counts[HSC_COUNT];
	uint16_t temp_counts[HSC_COUNT];
	hsc_status_t status_arr[HSC_COUNT];

	if (HSC_ReadAllEx(pressure_counts, temp_counts, status_arr) == HAL_OK) {
		for (uint8_t i = 0; i < PRESS_CTRL_CH_NUM; i++) {
			pressCtrl[i].latestPress = HSC_CountsToPressure_mbar(pressure_counts[i]);
		}
		inputPress = HSC_SourceCountsToPressure_mbar(pressure_counts[5]);
	}
}

static void PressCtrl_StateMachine(PressCtrlCmd_t cmd)
{
	for (uint8_t i = 0; i < PRESS_CTRL_CH_NUM; i++) {
		if (!isChEnable(i, cmd.chSet)) {
			continue;
		}
		switch (cmd.cmdType) {
		case PRESS_CTRL_STOP:
			if (pressCtrl[i].state == PRESS_CTRL_RUNNING) {
				stopPressCtrl(i);
				pressCtrl[i].state = PRESS_CTRL_IDLE;
			}
			break;

		case PRESS_CTRL_START:
			if (pressCtrl[i].state == PRESS_CTRL_IDLE) {
				startPressCtrl(i, cmd.target[i]);
				pressCtrl[i].state = PRESS_CTRL_RUNNING;
			}
			break;

		case PRESS_CTRL_SET_TARGET:
			changePressCtrlTarget(i, cmd.target[i]);
			break;

		case PRESS_CTRL_SET_PI:
			PID_SetPIParas(&pressCtrl[i].pid, cmd.kp, cmd.ki, cmd.feedforward);
			break;

		case PRESS_CTRL_RESET:
			if (pressCtrl[i].state == PRESS_CTRL_FAULT) {
				resetPressCtrl(i);
				pressCtrl[i].state = PRESS_CTRL_IDLE;
			}
			break;
		default:
			break;
		}
	}
}

static void PressCtrl_Update()
{
	for (uint8_t i = 0; i < PRESS_CTRL_CH_NUM; i++) {
		if (pressCtrl[i].state == PRESS_CTRL_RUNNING) {	// In press Ctrl
			uint32_t now = osKernelGetTickCount();
			float dt = ((float)(now - pressCtrl[i].lastControlTime)) / (float)osKernelGetTickFreq();
			if (dt <= 0.0f) dt = 0.001f;
			pressCtrl[i].lastControlTime = now;

			float out = PI_Compute(&(pressCtrl[i].pid), pressCtrl[i].target, pressCtrl[i].latestPress, dt);

			// Update control output
			PropoValveDrive_SetAndUpdate(i, (uint16_t)out);
		}
	}
}

static void PressCtrl_Task(void *arg)
{
	PressCtrlCmd_t cmd;

	for (; ;) {
		updateMeasuredPress();

		if (osMessageQueueGet(pressCtrlCmdQueue, &cmd, NULL, 0) == osOK) {
			PressCtrl_StateMachine(cmd);
		}

		PressCtrl_Update();

		osDelay(150);
	}
}


void PressCtrl_Start(uint8_t ch, uint16_t target[PRESS_CTRL_CH_NUM])
{
	PressCtrlCmd_t cmd = {.cmdType = PRESS_CTRL_START, .chSet = ch};
	for (uint8_t i = 0; i < PRESS_CTRL_CH_NUM; i++) {
		cmd.target[i] = target[i];
	}
	if (osMessageQueuePut(pressCtrlCmdQueue, &cmd, 0, 100) != osOK) {
		LOG_WARNING("Send start press control command FAILED!");
	}
}

void PressCtrl_SetTarget(uint8_t ch, uint16_t target[PRESS_CTRL_CH_NUM])
{
	PressCtrlCmd_t cmd = {.cmdType = PRESS_CTRL_SET_TARGET, .chSet = ch};
	for (uint8_t i = 0; i < PRESS_CTRL_CH_NUM; i++) {
		cmd.target[i] = target[i];
	}
	if (osMessageQueuePut(pressCtrlCmdQueue, &cmd, 0, 100) != osOK) {
		LOG_WARNING("Send set press control target command FAILED!");
	}
}

void PressCtrl_SetPI(uint8_t ch, uint16_t kp_x100, uint16_t ki_x100, uint16_t ff)
{
	PressCtrlCmd_t cmd = {.cmdType = PRESS_CTRL_SET_PI, .chSet = ch};
	cmd.kp = (float)kp_x100 / 100.0;
	cmd.ki = (float)ki_x100 / 100.0;
	cmd.feedforward = (float)ff;
	if (osMessageQueuePut(pressCtrlCmdQueue, &cmd, 0, 100) != osOK) {
		LOG_WARNING("Send set press PI parameters target command FAILED!");
	}
}


void PressCtrl_Stop(uint8_t ch)
{
	PressCtrlCmd_t cmd = {.cmdType = PRESS_CTRL_STOP, .chSet = ch, .target = {0}};
	if (osMessageQueuePut(pressCtrlCmdQueue, &cmd, 0, 100) != osOK) {
		LOG_WARNING("Send stop press control command FAILED!");
	}
}

void PressCtrl_StartTask()
{
	pressCtrlCmdQueue = osMessageQueueNew(PRESS_CTRL_CMD_QUEUE_SIZE, sizeof(PressCtrlCmd_t), NULL);
	if (pressCtrlCmdQueue == NULL) {
		LOG_ERROR("Create press control command queue FAILED!");
	}

	const osThreadAttr_t taskAttr = {.name = "PressControl", .priority=osPriorityNormal, .stack_size=1024};
	pressCtrlThread = osThreadNew(PressCtrl_Task, NULL, &taskAttr);
	if (pressCtrlThread == NULL) {
		LOG_ERROR("Create press control thread FAILED!");
	} else {
		LOG_INFO("Create press control thread OK");
	}
}

float PressCtrl_GetLatestPress(uint8_t ch)
{
	if (ch >= PRESS_CTRL_CH_NUM) {
		return 0.0;
	}
	return pressCtrl[ch].latestPress;
}

float PressCtrl_GetInputPress()
{
	return inputPress;
}

PressCtrl_State_t PressCtrl_GetStatus(uint8_t ch)
{
	if (ch >= PRESS_CTRL_CH_NUM) {
		return PRESS_CTRL_IDLE;
	}
	return pressCtrl[ch].state;
}

uint16_t PressCtrl_GetTarget(uint8_t ch)
{
	if (ch >= PRESS_CTRL_CH_NUM) {
		return 0;
	}
	return pressCtrl[ch].target;
}

float PressCtrl_GetKp(uint8_t ch)
{
	return pressCtrl[ch].pid.Kp;
}

float PressCtrl_GetKi(uint8_t ch)
{
	return pressCtrl[ch].pid.Ki;
}

float PressCtrl_GetFF(uint8_t ch)
{
	return pressCtrl[ch].pid.feedforward;
}


