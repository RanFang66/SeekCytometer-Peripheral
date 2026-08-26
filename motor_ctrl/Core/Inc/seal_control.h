/*
 * seal_motor_control.h
 *
 *  Created on: 2025年12月2日
 *      Author: ranfa
 */

#ifndef INC_SEAL_CONTROL_H_
#define INC_SEAL_CONTROL_H_

#include "gpio.h"
#include "adc.h"

typedef enum {
	SEAL_IDLE = 0,
	SEAL_PUSHING,
	SEAL_PUSHED,
	SEAL_RELEASING,
	SEAL_RELEASED,
	SEAL_FAULT,
} SealStatus_t;

typedef enum {
	SEAL_CMD_STOP = 0,
	SEAL_CMD_PUSH,
	SEAL_CMD_RELEASE,
	SEAL_CMD_RESET,
} SealCommand_t;

/* H-bridge drive direction. Only the two modes required by the hardware are
 * implemented (see the H-bridge function table):
 *   SEAL_DIR_FORWARD : IN1 = PWM, IN2 = 0  -> forward, fast decay
 *   SEAL_DIR_REVERSE : IN1 = PWM, IN2 = 1  -> reverse, slow decay
 */
typedef enum {
	SEAL_DIR_FORWARD = 0,
	SEAL_DIR_REVERSE,
} SealMotorDir_t;


typedef struct {
	SealMotorDir_t		dir;			// Last commanded H-bridge direction
	uint16_t			motorSpeed;		// Last commanded duty, 0 ~ SEAL_MOTOR_PWM_MAX

	SealStatus_t 		status;
	uint16_t 			pushSpeed;
	uint16_t			releaseSpeed;
	uint16_t			motorCurrentFaultThresh;
	uint16_t			releaseCurrentThresh;
	uint16_t			pushTimeLimit;
	uint16_t			pushMinTimeSpan;
	uint16_t			pushExtraTimeSpan;		// Keep pushing this long after the sensor triggers
	uint16_t			releaseMinTimeSpan;
	uint16_t			releaseTimeLimit;
	uint32_t			pushStartTimestamp;
	uint32_t			pushSensorTimestamp;	// Tick the push sensor first latched
	uint8_t				pushSensorLatched;		// Sensor seen, running out the extra time
	uint32_t			releaseStartTime;
} SealCtrlCtx_t;

#define SEAL_CMD_QUEUE_SIZE				(8)



/*Experiment results:
 * Released to sensor position, the max motor current is 70~80
 * When motor is stalling to push, the motor stall current is 400~500
 * So the suggested pushed motor current threshold value is 150~350
 * */
#define DEFAULT_PUSH_SPEED				(6000)
#define DEFAULT_RELEASE_SPEED 			(6000)
#define DEFAULT_PUSH_TIME_LIMIT			(6000)
#define DEFAULT_RELEASE_TIME_LIMIT		(6000)
#define DEFAULT_RELEASE_CURRENT_THRESH	(350)
#define DEFAULT_MOTOR_FAULT_THRESH		(2000)
#define DEFAULT_PUSH_MIN_TIME			(3000)
/* The seal is seated by pushing a little further after the position sensor
 * triggers, instead of stopping on the sensor edge. */
#define DEFAULT_PUSH_EXTRA_TIME			(1000)
#define DEFAULT_RELEASE_MIN_TIME		(3000)

void SealCtrl_Init();
SealStatus_t SealCtrl_GetStatus();
uint8_t SealCtrl_SealPushed();
uint8_t SealCtrl_SealReleased();
void sealCtrl_SetMotorFaultThresh(uint16_t thresh);
void sealCtrl_SetMotorPushedThresh(uint16_t thresh);
void SealCtrl_SetPushSpeed(uint16_t speed);
void SealCtrl_SetReleaseSpeed(uint16_t speed);
void SealCtrl_SetPushTimeLimit(uint16_t timeout);
void SealCtrl_SetPushExtraTime(uint16_t ms);
void SealCtrl_SetReleaseTimeLimit(uint16_t timeout);
uint16_t SealCtrl_GetPushSpeed();
uint16_t SealCtrl_GetReleaseSpeed();
uint16_t SealCtrl_GetPushTimeLimit();
uint16_t SealCtrl_GetPushExtraTime();
uint16_t SealCtrl_GetReleaseTimeLimit();
uint16_t SealCtrl_GetFaultCurrThresh();
uint16_t SealCtrl_GetPushedCurrThresh();
void SealCtrl_Push();
void SealCtrl_Release();
void SealCtrl_Stop();
void SealCtrl_Reset();
void SealCtrl_StartTask();

/* --- Bench diagnostics ---------------------------------------------------
 * Drive the H-bridge inputs directly, bypassing the command queue and the
 * state machine (status is forced to SEAL_IDLE so the task keeps its hands
 * off). in1Duty is the fraction of the period IN1 is HIGH, 0 ~ 10000.
 *   in1Duty=d, in2=0 -> forward, fast decay, duty d
 *   in1Duty=d, in2=1 -> reverse, slow decay, duty (10000 - d)
 * ------------------------------------------------------------------------*/
void SealCtrl_RawDrive(uint16_t in1Duty, uint8_t in2Level);
uint16_t SealCtrl_GetIn1Compare(void);	// raw TIM CCR value
uint8_t SealCtrl_GetIn1Level(void);		// IN1 (PA10) pin state sampled from IDR
uint8_t SealCtrl_GetIn2Level(void);		// IN2 (PA8) pin state
uint8_t SealCtrl_GetPowerLevel(void);	// PC7 H-bridge power enable state



#endif /* INC_SEAL_CONTROL_H_ */
