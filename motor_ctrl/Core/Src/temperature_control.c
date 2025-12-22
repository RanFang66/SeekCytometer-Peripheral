/*
 * temperature_control.c
 *
 *  Created on: 2025年12月3日
 *      Author: ranfa
 */

#include "temperature_control.h"
#include "bsp_tim.h"
#include "bsp_gpio.h"
#include "bsp_adc.h"
#include "ntc_sensor.h"
#include "debug_shell.h"
#include "cmsis_os2.h"

static Peltier_t peltier_1 = {
		.htim = &COOLER_1_TIM,
		.pwmChannel = COOLER_1_PWM_CH,
		.pwmVal = 0,
		.status = 0,
};

static Peltier_t peltier_2 = {
		.htim = &COOLER_2_TIM,
		.pwmChannel = COOLER_2_PWM_CH,
		.pwmVal = 0,
		.status = 0,
};

static Fan_t fan_1 = {
		.htim = &FAN_1_TIM,
		.pwmChannel = FAN_1_PWM_CH,
		.pwmVal = 0,
		.status = 0,
};

static Fan_t fan_2 = {
		.htim = &FAN_2_TIM,
		.pwmChannel = FAN_2_PWM_CH,
		.pwmVal = 0,
		.status = 0,
};

static Fan_t fan_3 = {
		.htim = &FAN_3_TIM,
		.pwmChannel = FAN_3_PWM_CH,
		.pwmVal = 0,
		.status = 0,
};

static PID_HandleTypeDef pid;

static Fan_t fan_4 = {
		.htim = &FAN_4_TIM,
		.pwmChannel = FAN_4_PWM_CH,
		.pwmVal = 0,
		.status = 0,
};


static TempCtrlCtx_t 		tempCtrl;
static osMessageQueueId_t 	tempCtrlCmdQueue = NULL;
static osThreadId_t 		tempCtrlThread = NULL;


void Peltier_SetPwmOutput(Peltier_t *p, uint16_t value)
{
	__HAL_TIM_SET_COMPARE(p->htim, p->pwmChannel, value);
	p->pwmVal = value;
}

void Peltier_Enable(Peltier_t *p)
{
	HAL_TIM_PWM_Start(p->htim, p->pwmChannel);
	p->status = 1;
}

void Peltier_Disable(Peltier_t *p)
{
	HAL_TIM_PWM_Stop(p->htim, p->pwmChannel);
	__HAL_TIM_SET_COMPARE(p->htim, p->pwmChannel, 0);
	p->pwmVal = 0;
	p->status = 0;
}

void Fan_Disable(Fan_t *f)
{
	HAL_TIM_PWM_Stop(f->htim, f->pwmChannel);
	__HAL_TIM_SET_COMPARE(f->htim, f->pwmChannel, 0);
	f->pwmVal = 0;
	f->status = 0;
}

void Fan_Enable(Fan_t *f)
{
	HAL_TIM_PWM_Start(f->htim, f->pwmChannel);
	f->status = 1;
}

void Fan_SetSpeed(Fan_t *f, uint16_t speed)
{
	__HAL_TIM_SET_COMPARE(f->htim, f->pwmChannel, speed);
	f->pwmVal = speed;
}


void Fan_SetPwmOutput(Fan_t *f, uint16_t value)
{
	__HAL_TIM_SET_COMPARE(f->htim, f->pwmChannel, value);
	f->pwmVal = value;
}



void TempCtrl_Init()
{
	tempCtrl.peltier[0] = &peltier_1;
	tempCtrl.peltier[1] = &peltier_2;
	tempCtrl.fan[0] = &fan_1;
	tempCtrl.fan[1] = &fan_2;
	tempCtrl.fan[2] = &fan_3;
	tempCtrl.fan[3] = &fan_4;

	PID_Init(&pid, DEFAULT_KP, DEFAULT_KI, DEFAULT_KD, DEFAULT_TAU, DEFAULT_OUT_MIN, DEFAULT_OUT_MAX, DEFAULT_STEP_MIN, DEFAULT_STEP_MAX);
	tempCtrl.pid = &pid;
	tempCtrl.temp_min = DEFAULT_TEMP_MIN;
	tempCtrl.temp_max = DEFAULT_TEMP_MAX;
	tempCtrl.temp_target = DEFAULT_TEMP_TARGET;
	tempCtrl.peltier_enable[0] = 1;
	tempCtrl.peltier_enable[1] = 1;
}


void TempCtrl_SetTarget(float target)
{
//	if (temp >= tempCtrl.temp_max || temp <= tempCtrl.temp_min) {
//		return;
//	}
//	tempCtrl.temp_target = temp;

	TempCtrlCmd_t cmd = {.cmdType = TEMP_CTRL_SET_TARGET, .targetTemp = target};
	osStatus_t st = osMessageQueuePut(tempCtrlCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send set temperature target command FAILED!");
	}
}



void TempCtrl_Start(float target)
{
	TempCtrlCmd_t cmd = {.cmdType = TEMP_CTRL_CMD_START, .targetTemp = target};

	osStatus_t st = osMessageQueuePut(tempCtrlCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send START temperature control command FAILED!");
	}
}

void TempCtrl_FanSet(uint8_t fanEnCh, uint16_t fanSpeed[COOL_FAN_NUM])
{
	TempCtrlCmd_t cmd = {.cmdType = TEMP_CTRL_FAN_SET, .fanEn = fanEnCh};
	for (uint8_t i = 0; i < COOL_FAN_NUM; ++i) {
		cmd.fanSpeed[i] = fanSpeed[i];
	}

	osStatus_t st = osMessageQueuePut(tempCtrlCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send fan set control command FAILED!");
	}
}


void TempCtrl_EnableFan(uint8_t fanCh)
{
	TempCtrlCmd_t cmd = {.cmdType = TEMP_CTRL_FAN_ENABLE, .fanEn = fanCh};

	osStatus_t st = osMessageQueuePut(tempCtrlCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send fan enable command FAILED!");
	}
}

void TempCtrl_DisableFan(uint8_t fanCh)
{
	TempCtrlCmd_t cmd = {.cmdType = TEMP_CTRL_FAN_DISABLE, .fanEn = fanCh};

	osStatus_t st = osMessageQueuePut(tempCtrlCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send fan disable command FAILED!");
	}
}

void TempCtrl_SetFanSpeed(uint8_t fanCh, uint16_t speed)
{
	TempCtrlCmd_t cmd = {.cmdType = TEMP_CTRL_FAN_SET_SPEED, .fanEn = fanCh};
	for (uint8_t i = 0; i < COOL_FAN_NUM; ++i) {
		if (fanCh & (0x01 << i)) {
			cmd.fanSpeed[i] = speed;
		}
	}


	osStatus_t st = osMessageQueuePut(tempCtrlCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send fan set speed command FAILED!");
	}
}

void TempCtrl_Stop()
{
	TempCtrlCmd_t cmd = {.cmdType = TEMP_CTRL_CMD_STOP};
	osStatus_t st = osMessageQueuePut(tempCtrlCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send STOP temperature control command FAILED!");
	}
}

TempCtrlStatus_t TempCtrl_GetStatus()
{
	return tempCtrl.status;
}

float TempCtrl_GetTempTarget()
{
	return tempCtrl.temp_target;
}

float TempCtrl_GetTempLatest()
{
	return tempCtrl.temp_latest;
}

float TempCtrl_GetKp()
{
	return tempCtrl.pid->Kp;
}

float TempCtrl_GetKi()
{
	return tempCtrl.pid->Ki;
}

uint8_t TempCtrl_GetFanStatus(uint8_t id)
{
	return tempCtrl.fan[id]->status;
}

uint16_t TempCtrl_GetFanSpeed(uint8_t id)
{
	return tempCtrl.fan[id]->pwmVal;
}


uint8_t TempCtrl_GetPeltierStatus(uint8_t id)
{
	return tempCtrl.peltier[id]->status;
}

uint16_t TempCtrl_GetPeltierOutput(uint8_t id)
{
	return tempCtrl.peltier[id]->pwmVal;
}


void TempCtrl_Reset()
{
	TempCtrlCmd_t cmd = {.cmdType = TEMP_CTRL_CMD_RESET};
	osStatus_t st = osMessageQueuePut(tempCtrlCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send RESET temperature control command FAILED!");
	}
}



static void UpdateTempCtrlOutput(uint16_t output)
{
	for (uint8_t i = 0; i < PELTIER_NUM; i++) {
		if (tempCtrl.peltier_enable[i]) {
			Peltier_SetPwmOutput(tempCtrl.peltier[i], output);
		}
	}
}

static void StartTempCtrl(float temp)
{
//	// Enable cool fan
//	for (uint8_t i = 0; i < COOL_FAN_NUM; i++) {
//		if ((enFanCh & ((uint8_t)0x01 << i))) {
//			Fan_SetPwmOutput(tempCtrl.fan[i], fanSpeed[i]);
//			Fan_Enable(tempCtrl.fan[i]);
//		} else {
//			Fan_Disable(tempCtrl.fan[i]);
//		}
//	}

	// Enable peltier
	for (uint8_t i = 0; i < PELTIER_NUM; i++) {
		if (tempCtrl.peltier_enable[i]) {
			Peltier_Enable(tempCtrl.peltier[i]);
		}
	}

	// Update Target
	if (temp < tempCtrl.temp_max && temp > tempCtrl.temp_min) {
		tempCtrl.temp_target = temp;
	}


	// Reset pid
	PID_Reset(tempCtrl.pid);
}

static void StopTempCtrl(void)
{
	// Disable peltier
	for (uint8_t i = 0; i < PELTIER_NUM; i++) {
		Peltier_Disable(tempCtrl.peltier[i]);
	}

	// Disable fan
	for (uint8_t i = 0; i < COOL_FAN_NUM; ++i) {
		Fan_Disable(tempCtrl.fan[i]);
	}
}

static void CoolFanSetting(uint8_t fanEnCh, uint16_t fanSpeed[COOL_FAN_NUM])
{
	// Enable cool fan
	for (uint8_t i = 0; i < COOL_FAN_NUM; i++) {
		if ((fanEnCh & ((uint8_t)0x01 << i))) {
			Fan_SetPwmOutput(tempCtrl.fan[i], fanSpeed[i]);
			Fan_Enable(tempCtrl.fan[i]);
		} else {
			Fan_Disable(tempCtrl.fan[i]);
		}
	}
}


static void TempCtrl_Task(void *arg)
{
	uint32_t last_tick = osKernelGetTickCount();
	TempCtrlCmd_t cmd;
	for (;;) {
		if (osMessageQueueGet(tempCtrlCmdQueue, &cmd, NULL, 0) == osOK) {
			switch(cmd.cmdType) {
			case TEMP_CTRL_CMD_STOP:
				StopTempCtrl();
				if (tempCtrl.status != TEMP_CTRL_FAULT) {
					tempCtrl.status = TEMP_CTRL_IDLE;
				}
				break;
			case TEMP_CTRL_CMD_START:
				if (tempCtrl.status != TEMP_CTRL_FAULT) {
					StartTempCtrl(cmd.targetTemp);
					tempCtrl.status = TEMP_CTRL_RUNNING;
				}
				break;

			case TEMP_CTRL_FAN_SET:
				CoolFanSetting(cmd.fanEn, cmd.fanSpeed);
				break;

			case TEMP_CTRL_FAN_ENABLE:
				for (uint8_t i = 0; i < COOL_FAN_NUM; i++) {
					if (cmd.fanEn & ((uint8_t)0x01 << i)) {
						Fan_Enable(tempCtrl.fan[i]);
					}
				}
				break;

			case TEMP_CTRL_FAN_DISABLE:
				for (uint8_t i = 0; i < COOL_FAN_NUM; i++) {
					if (cmd.fanEn &((uint8_t)0x01 << i)) {
						Fan_Disable(tempCtrl.fan[i]);
					}
				}
				break;

			case TEMP_CTRL_FAN_SET_SPEED:
				for (uint8_t i = 0; i < COOL_FAN_NUM; i++) {
					if (cmd.fanEn &((uint8_t)0x01 << i)) {
						Fan_SetSpeed(tempCtrl.fan[i], cmd.fanSpeed[i]);
					}
				}
				break;

			case TEMP_CTRL_SET_TARGET:
				if (cmd.targetTemp < tempCtrl.temp_max && cmd.targetTemp > tempCtrl.temp_min) {
					tempCtrl.temp_target = cmd.targetTemp;
				}
				break;

			case TEMP_CTRL_CMD_RESET:
				if (tempCtrl.status == TEMP_CTRL_FAULT) {
					StopTempCtrl();
					tempCtrl.status = TEMP_CTRL_IDLE;
				}
				break;
			default:
				break;
			}
		}

		// Update measured temperature
		tempCtrl.temp_latest = NTC_ConvertToTemp(GetTempAdc());
//		if (tempCtrl.temp_latest >= tempCtrl.temp_max || tempCtrl.temp_latest <= tempCtrl.temp_min) {
//			tempCtrl.status = TEMP_CTRL_FAULT;
//		}

		// Update delta t
		uint32_t now = osKernelGetTickCount();
		float dt = ((float)(now - last_tick)) / (float)osKernelGetTickFreq();
		if (dt <= 0.0f) dt = 0.001f;
		last_tick = now;

		if (tempCtrl.status == TEMP_CTRL_RUNNING) {
			// Calculate PID output
			float out = PID_Compute(tempCtrl.pid, tempCtrl.temp_target, tempCtrl.temp_latest, dt);

			// Update control output
			UpdateTempCtrlOutput((uint16_t)out);
		}

		osDelay(200);
	}
}


void TempCtrl_StartTask()
{
	tempCtrlCmdQueue = osMessageQueueNew(TEMP_CTRL_CMD_QUEUE_SIZE, sizeof(TempCtrlCmd_t), NULL);
	if (tempCtrlCmdQueue == NULL) {
		LOG_ERROR("Create temperature control command queue FAILED!");
	}

	const osThreadAttr_t taskAttr = {.name = "TempControl", .priority=osPriorityNormal, .stack_size=512};
	tempCtrlThread = osThreadNew(TempCtrl_Task, NULL, &taskAttr);

	if (tempCtrlThread == NULL) {
		LOG_ERROR("Create temperature control thread FAILED!");
	} else {
		LOG_INFO("Create temperature control thread OK");
	}
}
