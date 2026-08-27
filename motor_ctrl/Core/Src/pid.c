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

void PID_Init(PID_HandleTypeDef *pid, float Kp, float Ki, float Kd, float tau, float feedforwardCoee,
		float out_min, float out_max, float step_min, float step_max)
{
	pid->Kp = Kp;
	pid->Ki = Ki;
	pid->Kd = Kd;
	pid->tau = (tau >= 0.0f) ? tau: 0.0f;
	pid->feedforwardCoee = feedforwardCoee;
	pid->out_min = out_min;
	pid->out_max = out_max;
	pid->step_min = step_min;
	pid->step_max = step_max;
	pid->prev_error = 0.0f;
	pid->prev_prev_error = 0.0f;
	pid->prev_output = 0.0f;
	pid->u_fb = 0.0f;
	pid->deltaD_f = 0.0f;
	pid->last_delta_u = 0.0f;
	pid->dbg_dP = 0.0f;
	pid->dbg_dI = 0.0f;
	pid->dbg_dD = 0.0f;
	pid->dbg_ff = 0.0f;
}

void PID_SetTunings(PID_HandleTypeDef *pid, float Kp, float Ki, float Kd, float tau, float feedforwardCoee)
{
	pid->Kp = Kp;
	pid->Ki = Ki;
	pid->Kd = Kd;
	pid->tau = (tau >= 0.0f) ? tau: 0.0f;
	pid->feedforwardCoee = feedforwardCoee;
}

void PID_SetOutputLimits(PID_HandleTypeDef *pid, float out_min, float out_max)
{
	if (out_max < out_min) return;
	pid->out_min = out_min;
	pid->out_max = out_max;
	pid->prev_output = clampf(pid->prev_output, out_min, out_max);
	pid->u_fb = clampf(pid->u_fb, out_min, out_max);
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
	pid->u_fb = 0.0f;
	pid->deltaD_f = 0.0f;
	pid->last_delta_u = 0.0f;
	pid->dbg_dP = 0.0f;
	pid->dbg_dI = 0.0f;
	pid->dbg_dD = 0.0f;
	pid->dbg_ff = 0.0f;
}

/*
  Compute incremental PID with:
   ΔP = Kp * (e[k] - e[k-1])
   ΔI = Ki * e[k] * dt
   ΔD_raw = Kd * (e[k] - 2e[k-1] + e[k-2]) / dt
  derivative is low-pass filtered: deltaD_f = deltaD_f_prev + alpha*(deltaD_raw - deltaD_f_prev)

  The increments accumulate into pid->u_fb only. The feed-forward is a static
  offset for the current operating point, so it is added once at output time:

      u = clamp(u_fb + ff)

  Adding ff to the increment instead (u = prev_output + delta + ff, with
  prev_output already containing the previous ff) would re-apply it every cycle
  and make u monotonically increasing whenever ff > -step_min - the output then
  pins at out_max and can never come back down.
*/
float PID_Compute(PID_HandleTypeDef *pid, float setpoint, float feedback, float feedforward, float dt)
{
    if (dt <= 0.0f) {
        return pid->prev_output;
    }

    float error = setpoint - feedback;

    float deltaP = pid->Kp * (error - pid->prev_error);
    float deltaI = pid->Ki * error * dt;

    float deltaD_raw = pid->Kd * (error - 2.0f*pid->prev_error + pid->prev_prev_error) / dt;

    /* Limit derivative */
    float max_abs_deltaD = pid->step_max * 3.0f;
    deltaD_raw = clampf(deltaD_raw, -max_abs_deltaD, max_abs_deltaD);

    /* Low pass filter for derivative */
    float deltaD_f = deltaD_raw;
    if (pid->tau > 0.0f) {
        float alpha = dt / (pid->tau + dt);
        deltaD_f = pid->deltaD_f + alpha * (deltaD_raw - pid->deltaD_f);
    }

    /* Feed-forward: static offset, never accumulated */
    float ff = clampf(feedforward, pid->out_min, pid->out_max);

    /* Limit step increment, then accumulate into the feedback state */
    float delta = clampf(deltaP + deltaI + deltaD_f, pid->step_min, pid->step_max);
    pid->u_fb += delta;

    /* Back-calculation anti-windup: bound the state itself to the headroom left
     * by the feed-forward. While the output is saturated u_fb stops at
     * (out_max - ff) instead of winding past it, so the moment the error changes
     * sign the P and I terms pull u_fb - and the output - straight back down. */
    pid->u_fb = clampf(pid->u_fb, pid->out_min - ff, pid->out_max - ff);

    /* Limit output */
    float u = clampf(pid->u_fb + ff, pid->out_min, pid->out_max);

    /* Update states */
    pid->last_delta_u = u - pid->prev_output;
    pid->prev_prev_error = pid->prev_error;
    pid->prev_error = error;
    pid->prev_output = u;
    pid->deltaD_f = deltaD_f;

    /* Diagnostics snapshot */
    pid->dbg_dP = deltaP;
    pid->dbg_dI = deltaI;
    pid->dbg_dD = deltaD_f;
    pid->dbg_ff = ff;

    return u;
}
