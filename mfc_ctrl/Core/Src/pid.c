/*
 * pid.c
 *
 *  Created on: 2025年12月3日
 *      Author: ranfa
 */

#include "pid.h"

static inline float clampf(float v, float lo, float hi)
{
    if (v > hi) return hi;
    if (v < lo) return lo;
    return v;
}

void PID_Init(PID_HandleTypeDef *pid, float Kp, float Ki, float Kd, float tau,
		float out_min, float out_max, float step_min, float step_max, float feedForward)
{
	pid->Kp = Kp;
	pid->Ki = Ki;
	pid->Kd = Kd;
	pid->tau = (tau >= 0.0f) ? tau: 0.0f;
	pid->out_min = out_min;
	pid->out_max = out_max;
	pid->step_min = step_min;
	pid->step_max = step_max;
	pid->feedforward = feedForward;
	pid->prev_error = 0.0f;
	pid->prev_prev_error = 0.0f;
	pid->prev_output = 0.0f;
	pid->deltaD_f = 0.0f;
	pid->last_delta_u = 0.0f;
}

void PID_SetTunings(PID_HandleTypeDef *pid, float Kp, float Ki, float Kd, float tau)
{
	pid->Kp = Kp;
	pid->Ki = Ki;
	pid->Kd = Kd;
	pid->tau = (tau >= 0.0f) ? tau: 0.0f;
}

void PID_SetPIParas(PID_HandleTypeDef *pid, float kp, float ki, float ff)
{
	pid->Kp = kp;
	pid->Ki = ki;
	pid->feedforward = ff;
}

void PID_SetOutputLimits(PID_HandleTypeDef *pid, float out_min, float out_max)
{
	if (out_max < out_min) return;
	pid->out_min = out_min;
	pid->out_max = out_max;
	pid->prev_output = clampf(pid->prev_output, out_min, out_max);
}

void PID_SetStepLimits(PID_HandleTypeDef *pid, float step_min, float step_max)
{
	if (step_max < step_min || step_max < 1e-6f || step_min > -1e-6f)
		return;
	pid->step_min = step_min;
	pid->step_max = step_max;
	pid->last_delta_u = clampf(pid->last_delta_u, step_min, step_max);
}

void PID_Reset(PID_HandleTypeDef *pid)
{
	pid->prev_error = 0.0f;
	pid->prev_prev_error = 0.0f;
	pid->prev_output = 0.0f;
	pid->deltaD_f = 0.0f;
	pid->last_delta_u = 0.0f;
}

/*
  Compute incremental PID with:
   ΔP = Kp * (e[k] - e[k-1])
   ΔI = Ki * e[k] * dt
   ΔD_raw = Kd * (e[k] - 2e[k-1] + e[k-2]) / dt
  derivative is low-pass filtered: deltaD_f = deltaD_f_prev + alpha*(deltaD_raw - deltaD_f_prev)
*/
float PID_Compute(PID_HandleTypeDef *pid, float setpoint, float feedback, float dt)
{
    if (dt <= 0.0f) {
        return pid->prev_output;
    }

    float error = setpoint - feedback;

    float deltaP = pid->Kp * (error - pid->prev_error);
    float deltaI = pid->Ki * error * dt;

    float deltaD_raw = 0.0f;
    if (dt > 0.0f) {
        deltaD_raw = pid->Kd * (error - 2.0f*pid->prev_error + pid->prev_prev_error) / dt;
    }

    /* Limit derivative */
    float max_abs_deltaD = pid->step_max * 3.0f;
    deltaD_raw = clampf(deltaD_raw, -max_abs_deltaD, max_abs_deltaD);

    /* Low pass filter for derivative */
    float deltaD_f = deltaD_raw;
    if (pid->tau > 0.0f) {
        float alpha = dt / (pid->tau + dt);
        deltaD_f = pid->deltaD_f + alpha * (deltaD_raw - pid->deltaD_f);
    }


    float tentative_delta_raw = deltaP + deltaI + deltaD_f;

    /* Conditional integral */
    if ((pid->prev_output >= pid->out_max && deltaI > 0.0f) ||
        (pid->prev_output <= pid->out_min && deltaI < 0.0f)) {
        deltaI = 0.0f;
        tentative_delta_raw = deltaP + deltaI + deltaD_f;
    }

    /* Limit step increment */
    float tentative_delta = clampf(tentative_delta_raw, pid->step_min, pid->step_max);
    float tentative_u = pid->prev_output + tentative_delta;

    /* Limit output */
    float u = clampf(tentative_u, pid->out_min, pid->out_max);

    /* Update states */
    pid->last_delta_u = u - pid->prev_output;
    pid->prev_prev_error = pid->prev_error;
    pid->prev_error = error;
    pid->prev_output = u;
    pid->deltaD_f = deltaD_f;

    return u + pid->feedforward;
}


/*
  Compute incremental PI with:
   ΔP = Kp * (e[k] - e[k-1])
   ΔI = Ki * e[k] * dt
*/
float PI_Compute(PID_HandleTypeDef *pid, float setpoint, float feedback, float dt)
{
    if (dt <= 0.0f) {
        return pid->prev_output;
    }

    float error = setpoint - feedback;

    float deltaP = pid->Kp * (error - pid->prev_error);
    float deltaI = pid->Ki * error * dt;



    /* Low pass filter for derivative */
    float tentative_delta_raw = deltaP + deltaI;

    /* Conditional integral */
    if ((pid->prev_output >= pid->out_max && deltaI > 0.0f) ||
        (pid->prev_output <= pid->out_min && deltaI < 0.0f)) {
        deltaI = 0.0f;
        tentative_delta_raw = deltaP + deltaI;
    }

    /* Limit step increment */
    float tentative_delta = clampf(tentative_delta_raw, pid->step_min, pid->step_max);
    float tentative_u = pid->prev_output + tentative_delta;

    /* Limit output */
    float u = clampf(tentative_u, pid->out_min, pid->out_max);

    /* Update states */
    pid->last_delta_u = u - pid->prev_output;
    pid->prev_prev_error = pid->prev_error;
    pid->prev_error = error;
    pid->prev_output = u;
    pid->deltaD_f = 0;

    return u + pid->feedforward;
}
