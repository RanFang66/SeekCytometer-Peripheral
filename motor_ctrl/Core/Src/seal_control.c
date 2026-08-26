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

static SealCtrlCtx_t  		sealCtrlCtx;
static osMessageQueueId_t 	sealCmdQueue = NULL;
static osThreadId_t			sealThread = NULL;

/* --- Seal motor H-bridge drive layer -------------------------------------
 *
 * The seal motor is driven by an H-bridge with two inputs:
 *   IN1 = PA10, TIM1 CH3 PWM output   (SEAL_MOTOR_IN1_*)
 *   IN2 = PA8,  plain GPIO output     (SEAL_MOTOR_IN2_*)
 *
 * H-bridge logic table:
 *   IN1 IN2   OUT1 OUT2   function
 *    0   0     Z    Z     coast
 *    1   0     H    L     forward
 *    0   1     L    H     reverse
 *    1   1     L    L     brake
 *
 * Only two of the four PWM modes are usable here (IN2 is not a PWM capable
 * output), so the module implements:
 *   forward : IN1 = PWM, IN2 = 0  -> forward, fast decay
 *             drive is applied while IN1 is HIGH -> IN1 high duty = speed
 *   reverse : IN1 = PWM, IN2 = 1  -> reverse, slow decay
 *             drive is applied while IN1 is LOW (IN1 = 1 brakes)
 *                                          -> IN1 high duty = MAX - speed
 * ------------------------------------------------------------------------*/

/* Which H-bridge direction pushes the seal. Swap these two lines if the motor
 * turns the wrong way on the bench. */
#define SEAL_PUSH_DIR		SEAL_DIR_FORWARD
#define SEAL_RELEASE_DIR	SEAL_DIR_REVERSE

/* Dead time inserted when the drive direction is reversed */
#define SEAL_DIR_CHANGE_DELAY_MS	(10)

/* Set the fraction of the PWM period IN1 is driven HIGH.
 * TIM1 CH3 is configured with TIM_OCPOLARITY_LOW in tim.c, so the compare
 * register counts the LOW time and the value has to be inverted. */
static void SealPwm_SetIn1HighDuty(uint16_t duty)
{
	if (duty > SEAL_MOTOR_PWM_MAX) {
		duty = SEAL_MOTOR_PWM_MAX;
	}

#if SEAL_MOTOR_PWM_ACTIVE_LOW
	__HAL_TIM_SET_COMPARE(&SEAL_MOTOR_IN1_TIM, SEAL_MOTOR_IN1_PWM_CH, SEAL_MOTOR_PWM_MAX - duty);
#else
	__HAL_TIM_SET_COMPARE(&SEAL_MOTOR_IN1_TIM, SEAL_MOTOR_IN1_PWM_CH, duty);
#endif
}

static inline void SealIn2_Write(GPIO_PinState st)
{
	HAL_GPIO_WritePin(SEAL_MOTOR_IN2_GPIO, SEAL_MOTOR_IN2_PIN, st);
}

static inline void SealPower_Enable(void)
{
	HAL_GPIO_WritePin(SEAL_MOTOR_EN_GPIO, SEAL_MOTOR_EN_PIN, GPIO_PIN_SET);
}

static inline void SealPower_Disable(void)
{
	HAL_GPIO_WritePin(SEAL_MOTOR_EN_GPIO, SEAL_MOTOR_EN_PIN, GPIO_PIN_RESET);
}

/* IN1 = 0, IN2 = 0 -> outputs high impedance, motor coasts */
static void SealMotor_Coast(void)
{
	SealPwm_SetIn1HighDuty(0);
	SealIn2_Write(GPIO_PIN_RESET);
	sealCtrlCtx.motorSpeed = 0;
}

/* Apply a drive direction and duty. Passing through a safe state (coast for
 * forward, brake for reverse) before flipping IN2 avoids driving the bridge
 * straight from one direction into the other. */
static void SealMotor_Drive(SealMotorDir_t dir, uint16_t speed)
{
	uint8_t dirChanged = (dir != sealCtrlCtx.dir);

	if (speed > SEAL_MOTOR_PWM_MAX) {
		speed = SEAL_MOTOR_PWM_MAX;
	}

	if (dir == SEAL_DIR_FORWARD) {
		if (dirChanged) {
			SealPwm_SetIn1HighDuty(0);			// IN1 = 0
			SealIn2_Write(GPIO_PIN_RESET);		// (0, 0) coast
			osDelay(SEAL_DIR_CHANGE_DELAY_MS);
		} else {
			SealIn2_Write(GPIO_PIN_RESET);
		}
		SealPwm_SetIn1HighDuty(speed);			// forward, fast decay
	} else {
		if (dirChanged) {
			SealPwm_SetIn1HighDuty(SEAL_MOTOR_PWM_MAX);	// IN1 = 1
			SealIn2_Write(GPIO_PIN_SET);				// (1, 1) brake
			osDelay(SEAL_DIR_CHANGE_DELAY_MS);
		} else {
			SealIn2_Write(GPIO_PIN_SET);
		}
		SealPwm_SetIn1HighDuty(SEAL_MOTOR_PWM_MAX - speed);	// reverse, slow decay
	}

	sealCtrlCtx.dir = dir;
	sealCtrlCtx.motorSpeed = speed;
}

static void SealMotorRun(SealMotorDir_t dir, uint16_t speed)
{
	SealPower_Enable();
	SealMotor_Drive(dir, speed);
}

static void SealMotorPowerOff(void)
{
	SealMotor_Coast();
	SealPower_Disable();
}


void SealCtrl_Init()
{
	/* Keep the bridge unpowered and both inputs low until a command arrives.
	 * The PWM output is started once here and never stopped afterwards: stopping
	 * it would clear TIM1 MOE and leave PA10 floating (OSSI/OSSR are disabled),
	 * which the H-bridge input must not see. */
	SealPower_Disable();
	SealIn2_Write(GPIO_PIN_RESET);
	SealPwm_SetIn1HighDuty(0);
	HAL_TIM_PWM_Start(&SEAL_MOTOR_IN1_TIM, SEAL_MOTOR_IN1_PWM_CH);

	sealCtrlCtx.dir = SEAL_DIR_FORWARD;
	sealCtrlCtx.motorSpeed = 0;
	sealCtrlCtx.status = SEAL_IDLE;
	sealCtrlCtx.pushSpeed = DEFAULT_PUSH_SPEED;
	sealCtrlCtx.releaseSpeed = DEFAULT_RELEASE_SPEED;
	sealCtrlCtx.pushTimeLimit = DEFAULT_PUSH_TIME_LIMIT;
	sealCtrlCtx.releaseTimeLimit = DEFAULT_RELEASE_TIME_LIMIT;
	sealCtrlCtx.releaseCurrentThresh = DEFAULT_RELEASE_CURRENT_THRESH;
	sealCtrlCtx.motorCurrentFaultThresh = DEFAULT_MOTOR_FAULT_THRESH;
	sealCtrlCtx.pushMinTimeSpan = DEFAULT_PUSH_MIN_TIME;
	sealCtrlCtx.pushExtraTimeSpan = DEFAULT_PUSH_EXTRA_TIME;
	sealCtrlCtx.pushSensorLatched = 0;
	sealCtrlCtx.pushSensorTimestamp = 0;
	sealCtrlCtx.releaseMinTimeSpan = DEFAULT_RELEASE_MIN_TIME;
}

uint8_t SealCtrl_SealPushed()
{
	GPIO_PinState st = HAL_GPIO_ReadPin(SEAL_PUSHED_GPIO, SEAL_PUSHED_PIN);
	return (uint8_t)st;
}


uint8_t SealCtrl_SealReleased()
{
	GPIO_PinState st = HAL_GPIO_ReadPin(SEAL_PUSHED_GPIO, SEAL_PUSHED_PIN);
	return (uint8_t)st;
}

// Wrap H-bridge operation to seal control operation
#define SealMotorStop() 	SealMotorPowerOff()
#define SealMotorPush() 	SealMotorRun(SEAL_PUSH_DIR, sealCtrlCtx.pushSpeed)
#define SealMotorRelease() 	SealMotorRun(SEAL_RELEASE_DIR, sealCtrlCtx.releaseSpeed)


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
		isPushed = SealCtrl_SealPushed();
		motorI = GetMotorCurrentAdc();
		if (motorI > maxI) {
			maxI = motorI;
		}
		// Fault Check
		if (motorI > sealCtrlCtx.motorCurrentFaultThresh && sealCtrlCtx.status != SEAL_FAULT) {
			SealMotorStop();
			sealCtrlCtx.status = SEAL_FAULT;
		}


//		isReleased = (motorI >= sealCtrlCtx.releaseCurrentThresh);
		isReleased = SealCtrl_SealReleased();
		currentTimeStamp = osKernelGetTickCount();
		// Handle position changed
		switch (sealCtrlCtx.status) {
		case SEAL_PUSHING:
			if (!sealCtrlCtx.pushSensorLatched) {
				// Still looking for the sensor. The min time span keeps a sensor that
				// is already covered at rest from latching before the seal has moved.
				if (currentTimeStamp > (sealCtrlCtx.pushStartTimestamp + sealCtrlCtx.pushMinTimeSpan) && isPushed) {
					sealCtrlCtx.pushSensorLatched = 1;
					sealCtrlCtx.pushSensorTimestamp = currentTimeStamp;
				} else if (sealCtrlCtx.pushStartTimestamp + sealCtrlCtx.pushTimeLimit < currentTimeStamp) {
					SealMotorStop();
					sealCtrlCtx.status = SEAL_FAULT;
				}
			} else if (currentTimeStamp > (sealCtrlCtx.pushSensorTimestamp + sealCtrlCtx.pushExtraTimeSpan)) {
				// Sensor seen: the motor kept pushing for pushExtraTimeSpan to seat the
				// seal, now stop. The push time limit no longer applies here - this
				// overrun is intended, and the over-current check still guards it.
				SealMotorStop();
				sealCtrlCtx.status = SEAL_PUSHED;
			}
			break;
		case SEAL_RELEASING:
			if (currentTimeStamp > (sealCtrlCtx.releaseStartTime + sealCtrlCtx.releaseMinTimeSpan) && isReleased) {
				SealMotorStop();
				sealCtrlCtx.status = SEAL_RELEASED;
			}
			if (currentTimeStamp > (sealCtrlCtx.releaseStartTime + sealCtrlCtx.releaseTimeLimit)) {
				SealMotorStop();
				sealCtrlCtx.status = SEAL_FAULT;
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
				if (sealCtrlCtx.status != SEAL_FAULT && sealCtrlCtx.status != SEAL_PUSHING) {
					sealCtrlCtx.pushSensorLatched = 0;
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
	sealCtrlCtx.releaseCurrentThresh = thresh;
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


void SealCtrl_SetPushExtraTime(uint16_t ms)
{
	sealCtrlCtx.pushExtraTimeSpan = ms;
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


uint16_t SealCtrl_GetPushExtraTime()
{
	return sealCtrlCtx.pushExtraTimeSpan;
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
	return sealCtrlCtx.releaseCurrentThresh;
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

/* --- Bench diagnostics, see seal_control.h ------------------------------- */
void SealCtrl_RawDrive(uint16_t in1Duty, uint8_t in2Level)
{
	sealCtrlCtx.status = SEAL_IDLE;		// keep the state machine out of the way
	SealPower_Enable();
	SealIn2_Write(in2Level ? GPIO_PIN_SET : GPIO_PIN_RESET);
	SealPwm_SetIn1HighDuty(in1Duty);

	sealCtrlCtx.dir = in2Level ? SEAL_DIR_REVERSE : SEAL_DIR_FORWARD;
	sealCtrlCtx.motorSpeed = in1Duty;
}

uint16_t SealCtrl_GetIn1Compare(void)
{
	return (uint16_t)__HAL_TIM_GET_COMPARE(&SEAL_MOTOR_IN1_TIM, SEAL_MOTOR_IN1_PWM_CH);
}

uint8_t SealCtrl_GetIn1Level(void)
{
	return (uint8_t)HAL_GPIO_ReadPin(SEAL_MOTOR_IN1_GPIO, SEAL_MOTOR_IN1_PIN);
}

uint8_t SealCtrl_GetIn2Level(void)
{
	return (uint8_t)HAL_GPIO_ReadPin(SEAL_MOTOR_IN2_GPIO, SEAL_MOTOR_IN2_PIN);
}

uint8_t SealCtrl_GetPowerLevel(void)
{
	return (uint8_t)HAL_GPIO_ReadPin(SEAL_MOTOR_EN_GPIO, SEAL_MOTOR_EN_PIN);
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
