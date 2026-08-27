/*
 * pid.h
 *
 *  Created on: 2025年12月3日
 *      Author: ranfa
 */

#ifndef INC_PID_H_
#define INC_PID_H_

#include <stdint.h>

typedef struct {
	float Kp;
	float Ki;
	float Kd;

	/* Derivative filter time constant*/
	float tau;

	/* Output limit */
	float out_min;
	float out_max;

	/* Step Limit */
	float step_min;
	float step_max;

	/* Previous states */
	float prev_error;
	float prev_prev_error;

	/* Last total output u, only used for reporting and for the dt <= 0 early
	 * return. This is NOT the controller state - u_fb below is. */
	float prev_output;

	/* Accumulator of the feedback (P+I+D) increments. The feed-forward term is a
	 * static offset added on top of it at output time, so it must never be
	 * accumulated here - doing so makes the output monotonically increasing. */
	float u_fb;

	/* feed forward */
	float feedforwardCoee;				// default: 0, do not enable feed forward

	/* Derivative filter state */
	float deltaD_f;

	float last_delta_u;

	/* Snapshot of the last computed terms, for tuning/diagnostics only */
	float dbg_dP;
	float dbg_dI;
	float dbg_dD;
	float dbg_ff;
} PID_HandleTypeDef;

void PID_Init(PID_HandleTypeDef *pid, float Kp, float Ki, float Kd, float tau, float feedforwardCoee, float out_min, float out_max, float step_min, float step_max);

void PID_SetTunings(PID_HandleTypeDef *pid, float Kp, float Ki, float Kd, float tau, float feedforwardCoee);

void PID_SetOutputLimits(PID_HandleTypeDef *pid, float out_min, float out_max);

void PID_SetStepLimits(PID_HandleTypeDef *pid, float step_min, float step_max);

void PID_Reset(PID_HandleTypeDef *pid);

/* feedforward is the static output offset for the current operating point. It is
 * computed by the caller in its own physical domain (see TempCtrl_Feedforward())
 * so that this module never has to guess the sign convention of the plant. */
float PID_Compute(PID_HandleTypeDef *pid, float setpoint, float feedback, float feedforward, float dt);



#endif /* INC_PID_H_ */
