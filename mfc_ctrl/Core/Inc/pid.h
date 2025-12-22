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


	float feedforward;
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
	float prev_output;

	/* Derivative filter state */
	float deltaD_f;

	float last_delta_u;
} PID_HandleTypeDef;;

void PID_Init(PID_HandleTypeDef *pid, float Kp, float Ki, float Kd, float tau, float out_min, float out_max, float step_min, float step_max, float feedForward);

void PID_SetTunings(PID_HandleTypeDef *pid, float Kp, float Ki, float Kd, float tau);

void PID_SetPIParas(PID_HandleTypeDef *pid, float kp, float ki, float ff);

void PID_SetOutputLimits(PID_HandleTypeDef *pid, float out_min, float out_max);

void PID_SetStepLimits(PID_HandleTypeDef *pid, float step_min, float step_max);

void PID_Reset(PID_HandleTypeDef *pid);

float PID_Compute(PID_HandleTypeDef *pid, float setpoint, float feedback, float dt);

float PI_Compute(PID_HandleTypeDef *pid, float setpoint, float feedback, float dt);



#endif /* INC_PID_H_ */
