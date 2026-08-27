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
		.phaseB = TEMP_PELTIER_INTERLEAVE,	// switches on the opposite half of the period
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


/* Timer clock feeding TIM12. On APB1 the timer clock is PCLK1 doubled whenever
 * the APB1 prescaler is not 1 (RM0090 clock tree) - 84 MHz with this board's
 * HCLK/4 setting. Read it from RCC instead of hard-coding, so a clock-tree change
 * in CubeMX cannot silently shift the PWM frequency. */
static uint32_t Peltier_TimerClk(void)
{
	uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
	return ((RCC->CFGR & RCC_CFGR_PPRE1) == 0U) ? pclk1 : (pclk1 * 2U);
}

/* PWM mode 1 is active while CNT < CCR (ON window at the start of the period),
 * PWM mode 2 while CNT >= CCR (ON window at the end). So a phase-B channel needs
 * the complementary compare value to end up with the same duty. Note that a duty
 * of 0 maps to CCR = 0 in mode 1 and CCR = TEMP_PWM_MAX (> ARR) in mode 2, both
 * of which are permanently inactive - and duty = TEMP_PWM_MAX is the reverse. */
static inline uint32_t Peltier_DutyToCompare(const Peltier_t *p, uint16_t duty)
{
	return p->phaseB ? (uint32_t)(TEMP_PWM_MAX - duty) : (uint32_t)duty;
}

void Peltier_SetPwmOutput(Peltier_t *p, uint16_t value)
{
	if (value > TEMP_PWM_MAX) {
		value = TEMP_PWM_MAX;
	}
	__HAL_TIM_SET_COMPARE(p->htim, p->pwmChannel, Peltier_DutyToCompare(p, value));
	p->pwmVal = value;
}

/* Puts the channel into its PWM mode and leaves it at 0% duty. Must run before
 * HAL_TIM_PWM_Start(); TIM12 is untouched in tim.c, this only rewrites the OCxM
 * bits of the channel through the normal HAL path so a CubeMX regeneration of
 * tim.c cannot undo it. */
static void Peltier_ConfigChannel(Peltier_t *p)
{
	TIM_OC_InitTypeDef oc = {0};
	oc.OCMode = p->phaseB ? TIM_OCMODE_PWM2 : TIM_OCMODE_PWM1;
	oc.Pulse = Peltier_DutyToCompare(p, 0);
	oc.OCPolarity = TIM_OCPOLARITY_HIGH;
	oc.OCFastMode = TIM_OCFAST_DISABLE;
	if (HAL_TIM_PWM_ConfigChannel(p->htim, &oc, p->pwmChannel) != HAL_OK) {
		LOG_ERROR("Config peltier PWM channel FAILED!");
	}
	p->pwmVal = 0;
}

void Peltier_Enable(Peltier_t *p)
{
	HAL_TIM_PWM_Start(p->htim, p->pwmChannel);
	p->status = 1;
}

void Peltier_Disable(Peltier_t *p)
{
	HAL_TIM_PWM_Stop(p->htim, p->pwmChannel);
	__HAL_TIM_SET_COMPARE(p->htim, p->pwmChannel, Peltier_DutyToCompare(p, 0));
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

	PID_Init(&pid, DEFAULT_KP, DEFAULT_KI, DEFAULT_KD, DEFAULT_TAU, DEFAULT_FEEDFORWARD_COEE, DEFAULT_OUT_MIN, DEFAULT_OUT_MAX, DEFAULT_STEP_MIN, DEFAULT_STEP_MAX);
	tempCtrl.pid = &pid;
	tempCtrl.temp_min = DEFAULT_TEMP_MIN;
	tempCtrl.temp_max = DEFAULT_TEMP_MAX;
	tempCtrl.temp_target = DEFAULT_TEMP_TARGET;
	tempCtrl.peltier_enable[0] = 1;
	tempCtrl.peltier_enable[1] = 1;

	/* Set the PWM modes (this is what interleaves the two peltiers) and the
	 * carrier frequency before anything starts driving them. */
	for (uint8_t i = 0; i < PELTIER_NUM; i++) {
		Peltier_ConfigChannel(tempCtrl.peltier[i]);
	}
	TempCtrl_SetPwmFreq(DEFAULT_PWM_FREQ_HZ);
}


uint32_t TempCtrl_SetPwmFreq(uint32_t hz)
{
	uint32_t timClk = Peltier_TimerClk();
	uint32_t maxHz = timClk / TEMP_PWM_MAX;

	if (hz > maxHz)          hz = maxHz;
	if (hz < MIN_PWM_FREQ_HZ) hz = MIN_PWM_FREQ_HZ;

	/* PSC+1 rounded to the nearest, so "temp -w 1000" lands as close as the
	 * divider allows instead of always rounding down. */
	uint32_t div = (timClk + (hz * TEMP_PWM_MAX) / 2U) / (hz * TEMP_PWM_MAX);
	if (div < 1U)      div = 1U;
	if (div > 0x10000U) div = 0x10000U;

	/* Both peltiers share TIM12, so one prescaler write covers both. It latches
	 * at the next update event; the compare values keep their meaning because the
	 * period (ARR) is unchanged, so the duty does not glitch. */
	__HAL_TIM_SET_PRESCALER(&COOLER_1_TIM, div - 1U);
	COOLER_1_TIM.Init.Prescaler = div - 1U;

	return timClk / div / TEMP_PWM_MAX;
}


uint32_t TempCtrl_GetPwmFreq(void)
{
	uint32_t div = (uint32_t)COOLER_1_TIM.Instance->PSC + 1U;
	return Peltier_TimerClk() / div / TEMP_PWM_MAX;
}


void TempCtrl_RawPwm(uint16_t percent)
{
	if (percent > 100U) {
		percent = 100U;
	}

	/* Take the loop out of RUNNING first so TempCtrl_Task() stops writing the
	 * compare registers, then drive both peltiers directly. Same shape as
	 * SealCtrl_RawDrive(). A FAULT is left alone - clear it with "temp -r". */
	if (tempCtrl.status == TEMP_CTRL_RUNNING) {
		tempCtrl.status = TEMP_CTRL_IDLE;
	}

	uint16_t duty = (uint16_t)(((uint32_t)percent * TEMP_PWM_MAX) / 100U);
	for (uint8_t i = 0; i < PELTIER_NUM; i++) {
		if (tempCtrl.peltier_enable[i]) {
			Peltier_Enable(tempCtrl.peltier[i]);
			Peltier_SetPwmOutput(tempCtrl.peltier[i], duty);
		}
	}
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
		if ((fanCh & ((uint8_t)0x01 << i))) {
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

float TempCtrl_GetTemp(int index)
{
	if (index < 0 || index >= TEMPERATURE_NTC_NUM) {
		return 0.0f;
	}
	return tempCtrl.temp_measured[index];
}




float TempCtrl_GetKp()
{
	return tempCtrl.pid->Kp;
}

float TempCtrl_GetKi()
{
	return tempCtrl.pid->Ki;
}

float TempCtrl_GetKd()
{
	return tempCtrl.pid->Kd;
}

float TempCtrl_GetFfCoee()
{
	return tempCtrl.pid->feedforwardCoee;
}

uint16_t TempCtrl_GetOutput()
{
	return (uint16_t)tempCtrl.pid->prev_output;
}

void TempCtrl_GetPidDebug(float *dP, float *dI, float *dD, float *ff)
{
	if (dP) *dP = tempCtrl.pid->dbg_dP;
	if (dI) *dI = tempCtrl.pid->dbg_dI;
	if (dD) *dD = tempCtrl.pid->dbg_dD;
	if (ff) *ff = tempCtrl.pid->dbg_ff;
}

/* Gains, not controller state - written straight into the handle instead of
 * going through the command queue, same as the getters above read it. */
void TempCtrl_SetTunings(float kp, float ki, float kd)
{
	PID_SetTunings(tempCtrl.pid, kp, ki, kd, tempCtrl.pid->tau, tempCtrl.pid->feedforwardCoee);
}

void TempCtrl_SetFeedforwardCoee(float coee)
{
	tempCtrl.pid->feedforwardCoee = coee;
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



/* Steady-state duty needed to hold the target against the ambient heat leak.
 * Computed here, in the real temperature domain, so the PID never has to deal
 * with the sign convention used at the call site below. */
static float TempCtrl_Feedforward(void)
{
	float delta = DEFAULT_AMBIENT_TEMP - tempCtrl.temp_target;
	return (delta > 0.0f) ? (tempCtrl.pid->feedforwardCoee * delta) : 0.0f;
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
//	for (uint8_t i = 0; i < COOL_FAN_NUM; ++i) {
//		Fan_Disable(tempCtrl.fan[i]);
//	}
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


/* Trip the peltiers if the heat sink runs away (fans off, blocked airflow...).
 * Without this the symptom is indistinguishable from a controller problem: the
 * output sits at full scale and the chamber refuses to cool. */
static void TempCtrl_CheckSink(uint32_t now)
{
	static uint32_t lastWarnTick = 0;
	float sink1 = tempCtrl.temp_measured[TEMPERATURE_SINK_1_INDEX];
	float sink2 = tempCtrl.temp_measured[TEMPERATURE_SINK_2_INDEX];
	float sink = (sink1 > sink2) ? sink1 : sink2;

	if (tempCtrl.status != TEMP_CTRL_RUNNING) {
		return;
	}

	if (sink >= SINK_TEMP_FAULT) {
		StopTempCtrl();
		tempCtrl.status = TEMP_CTRL_FAULT;
		LOG_ERROR("Heat sink over temperature: %d.%d degC, temperature control STOPPED",
				(int)sink, ((int)(sink * 10)) % 10);
	} else if (sink >= SINK_TEMP_WARN) {
		if (lastWarnTick == 0 || (now - lastWarnTick) >= SINK_WARN_PERIOD_MS) {
			lastWarnTick = now;
			LOG_WARNING("Heat sink hot: %d.%d degC, check the cooling fans",
					(int)sink, ((int)(sink * 10)) % 10);
		}
	} else {
		lastWarnTick = 0;
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
		tempCtrl.temp_measured[0] = NTC_ConvertToTemp(GetTempAdc_NTC1());
		tempCtrl.temp_measured[1] = NTC_ConvertToTemp(GetTempAdc_NTC2());
		tempCtrl.temp_measured[2] = NTC_ConvertToTemp(GetTempAdc_NTC3());
		tempCtrl.temp_measured[3] = NTC_ConvertToTemp(GetTempAdc_NTC4());

//		if (tempCtrl.temp_latest >= tempCtrl.temp_max || tempCtrl.temp_latest <= tempCtrl.temp_min) {
//			tempCtrl.status = TEMP_CTRL_FAULT;
//		}

		uint32_t now = osKernelGetTickCount();

		// Heat sink protection runs at the task rate, not the PID rate
		TempCtrl_CheckSink(now);

		// The PID runs slower than the task, see TEMP_CTRL_PID_PERIOD_MS
		if (tempCtrl.status == TEMP_CTRL_RUNNING &&
			(now - last_tick) >= TEMP_CTRL_PID_PERIOD_MS) {
			float dt = ((float)(now - last_tick)) / (float)osKernelGetTickFreq();
			last_tick = now;
			if (dt <= 0.0f) dt = 0.001f;

			/* The plant is reverse acting - more output means a lower chamber
			 * temperature - so both the setpoint and the feedback are negated to
			 * turn it into a normal direct-acting loop for the PID. The
			 * feed-forward is computed separately, in the real temperature
			 * domain, and is unaffected by this. */
			float out = PID_Compute(tempCtrl.pid,
									-tempCtrl.temp_target,
									-tempCtrl.temp_measured[TEMPERATURE_AIR_INDEX],
									TempCtrl_Feedforward(),
									dt);

			// Update control output
			UpdateTempCtrlOutput((uint16_t)out);
		} else if (tempCtrl.status != TEMP_CTRL_RUNNING) {
			// Keep the reference fresh so the first dt after a start is one period
			last_tick = now;
		}

		osDelay(400);
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
