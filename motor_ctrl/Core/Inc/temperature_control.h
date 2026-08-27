/*
 * temperature_control.h
 *
 *  Created on: 2025年12月3日
 *      Author: ranfa
 */

#ifndef INC_TEMPERATURE_CONTROL_H_
#define INC_TEMPERATURE_CONTROL_H_

#include "pid.h"
#include "tim.h"

typedef struct {
	TIM_HandleTypeDef 	*htim;
	uint32_t 			pwmChannel;
	uint8_t				status;
	/* Interleaving: a phaseB channel runs in PWM mode 2, so its ON window sits at
	 * the END of the period instead of the start. With one peltier on each phase
	 * the two no longer switch on together - see TEMP_PELTIER_INTERLEAVE. */
	uint8_t				phaseB;
	uint16_t			pwmVal;		// Requested duty, 0 ~ TEMP_PWM_MAX (not the raw CCR)
} Peltier_t;

typedef struct {
	TIM_HandleTypeDef 	*htim;
	uint32_t 			pwmChannel;
	uint8_t 			status;
	uint16_t			pwmVal;
} Fan_t;

#define PELTIER_NUM					(2)
#define COOL_FAN_NUM				(4)
#define TEMP_CTRL_CMD_QUEUE_SIZE 	(5)

typedef enum {
	TEMP_CTRL_IDLE = 0,
	TEMP_CTRL_RUNNING,
	TEMP_CTRL_FAULT
} TempCtrlStatus_t;

typedef enum {
	TEMP_CTRL_CMD_STOP = 0,
	TEMP_CTRL_CMD_START,
	TEMP_CTRL_SET_TARGET,
	TEMP_CTRL_FAN_SET,
	TEMP_CTRL_FAN_ENABLE,
	TEMP_CTRL_FAN_DISABLE,
	TEMP_CTRL_FAN_SET_SPEED,
	TEMP_CTRL_CMD_RESET,
} TempCmdType_t;


#define TEMPERATURE_NTC_NUM 			(4)
#define TEMPERATURE_AIR_INDEX			(0)
#define TEMPERATURE_SINK_1_INDEX		(1)
#define TEMPERATURE_SINK_2_INDEX		(2)
#define TEMPERATURE_OUTPUT_INDEX		(3)

typedef struct {
	TempCmdType_t 	cmdType;
	float 			targetTemp;
	uint8_t			fanEn;
	uint16_t 		fanSpeed[COOL_FAN_NUM];
} TempCtrlCmd_t;

typedef struct {
	PID_HandleTypeDef 	*pid;
	Peltier_t			*peltier[PELTIER_NUM];
	Fan_t				*fan[COOL_FAN_NUM];

	float 				temp_min;
	float 				temp_max;
	float 				temp_target;
	float 				temp_measured[TEMPERATURE_NTC_NUM];



	uint8_t 			peltier_enable[PELTIER_NUM];
//	uint8_t 			fan_enable[COOL_FAN_NUM];
//	uint16_t			fan_speed[COOL_FAN_NUM];
	TempCtrlStatus_t 	status;
} TempCtrlCtx_t;

/* The PID output drives TIM12 CH1/CH2 directly, whose ARR is 9999, so the output
 * span is 0 ~ 10000 counts = 0 ~ 100% duty. The gains have to be scaled to that
 * span: Kp is (full scale / proportional band), Ki is Kp / integral time.
 * Kp = 50 gave a proportional band of 10000/50 = 200 degC, which is why the P
 * term could not pull the output out of saturation. */
#define DEFAULT_KP					1000.0f		/* proportional band = TEMP_PWM_MAX / Kp */
#define DEFAULT_KI					15.0f		/* Kp / Ti (integral time) */
#define DEFAULT_KD					0.0f		/* unused for now */

#define DEFAULT_TAU					0.0f
#define DEFAULT_OUT_MIN				0.0f
#define DEFAULT_OUT_MAX 			10000.0f
#define DEFAULT_STEP_MIN 			-2000.0f	/* per control period, see below */
#define DEFAULT_STEP_MAX			2000.0f
#define DEFAULT_FEEDFORWARD_COEE 	150.0f		/* counts per degC, calibrate on the bench */

/* The task still runs every 200 ms so commands stay responsive, but the PID is
 * evaluated every TEMP_CTRL_PID_PERIOD_MS. The chamber time constant is minutes;
 * at 200 ms the per-sample temperature change is smaller than the NTC
 * quantisation (~45 LSB/degC) and the P term becomes pure noise. */
#define TEMP_CTRL_PID_PERIOD_MS		(1600)

/* Peltier PWM full scale == htim12.Init.Period + 1 (MX_TIM12_Init: Period = 9999).
 * DEFAULT_OUT_MAX above must stay equal to this - the PID output IS the compare
 * value. TIM12 is PWM1 / OCPOLARITY_HIGH, so CCR counts the HIGH time directly. */
#define TEMP_PWM_MAX				(10000U)

/* Both peltiers hang off TIM12. With both channels in PWM mode 1 they switch ON
 * at the same instant (CNT == 0), so the supply sees a single current step of
 * twice the amplitude - which is what couples into the huart3 line. Putting
 * cooler 2 into PWM mode 2 moves its ON window to the end of the period: at
 * CNT == 0 one channel rises while the other falls (the two steps largely
 * cancel) and the remaining two edges are spread apart in time. At 50% duty the
 * total current is even constant. Set to 0 to go back to both-in-phase. */
#define TEMP_PELTIER_INTERLEAVE		(1)

/* PWM frequency = timer clock / (PSC+1) / TEMP_PWM_MAX. TIM12 runs off APB1 x2 =
 * 84 MHz here, so PSC = 0 gives 8.4 kHz. Runtime-adjustable through "temp -w" for
 * bench work only - keep it high in normal use, a low carrier thermally cycles
 * the peltier and costs COP. */
#define DEFAULT_PWM_FREQ_HZ			(8400U)
#define MIN_PWM_FREQ_HZ				(20U)


/* No ambient NTC on this board yet - the heat leak model uses a constant. */
#define DEFAULT_AMBIENT_TEMP		25.0f

/* Heat-sink protection. The peltier COP collapses as the hot side heats up, so a
 * runaway sink shows up as "output saturated but the chamber is not cooling". */
#define SINK_TEMP_WARN				55.0f
#define SINK_TEMP_FAULT				70.0f
#define SINK_WARN_PERIOD_MS			(10000)


#define DEFAULT_TEMP_MIN	-10.0f
#define DEFAULT_TEMP_MAX	30.0f

#define DEFAULT_FAN_SPEED 	30000
#define DEFAULT_TEMP_TARGET	4.0f


void TempCtrl_Init();
void TempCtrl_SetTarget(float temp);
void TempCtrl_Start(float target);
void TempCtrl_FanSet(uint8_t fanEnCh, uint16_t fanSpeed[COOL_FAN_NUM]);
void TempCtrl_EnableFan(uint8_t fanCh);
void TempCtrl_DisableFan(uint8_t fanCh);
void TempCtrl_SetFanSpeed(uint8_t fanCh, uint16_t speed);

void TempCtrl_Stop();
void TempCtrl_Reset();
TempCtrlStatus_t TempCtrl_GetStatus();
float TempCtrl_GetTempTarget();
float TempCtrl_GetTemp(int index);
float TempCtrl_GetKp();
float TempCtrl_GetKi();
float TempCtrl_GetKd();
float TempCtrl_GetFfCoee();
uint16_t TempCtrl_GetOutput();
void TempCtrl_GetPidDebug(float *dP, float *dI, float *dD, float *ff);
void TempCtrl_SetTunings(float kp, float ki, float kd);
void TempCtrl_SetFeedforwardCoee(float coee);

/* PWM carrier frequency, shared by both peltiers (same timer).
 * Returns the frequency actually achieved after prescaler rounding. */
uint32_t TempCtrl_SetPwmFreq(uint32_t hz);
uint32_t TempCtrl_GetPwmFreq(void);

/* --- Bench diagnostics ---------------------------------------------------
 * Drive both peltiers open loop at a fixed duty, bypassing the PID (the loop is
 * forced out of RUNNING so the task stops writing the compare registers). Meant
 * for looking at the switching noise on a scope: 0% and 100% are the two static
 * cases, everything in between actually switches.
 * ------------------------------------------------------------------------*/
void TempCtrl_RawPwm(uint16_t percent);
uint8_t TempCtrl_GetFanStatus(uint8_t id);
uint16_t TempCtrl_GetFanSpeed(uint8_t id);
uint8_t TempCtrl_GetPeltierStatus(uint8_t id);
uint16_t TempCtrl_GetPeltierOutput(uint8_t id);

void TempCtrl_StartTask();

#endif /* INC_TEMPERATURE_CONTROL_H_ */
