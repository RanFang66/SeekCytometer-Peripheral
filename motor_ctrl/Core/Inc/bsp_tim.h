/*
 * bsp_tim.h
 *
 *  Created on: 2025年11月27日
 *      Author: ranfa
 */

#ifndef INC_BSP_TIM_H_
#define INC_BSP_TIM_H_

#include "tim.h"


/* BSP TIM PWM Configuration */
// churn motor: TIM9 CH1
#define CHURN_MOTOR_TIM		(htim9)
#define CHURN_MOTOR_PWM_CH	(TIM_CHANNEL_1)

// churn DC motor: software PWM time base, TIM7 (basic timer, no output pin)
#define CHURN_DC_MOTOR_TIM	(htim7)


// cover motor: TIM2 CH1
// DEPRECATED: cover mechanism removed by hardware revision, see ENABLE_COVER_CTRL
#define COVER_MOTOR_TIM 	(htim2)
#define COVER_MOTOR_PWM_CH	(TIM_CHANNEL_1)

// seal motor H-bridge IN1 (PWM input): TIM1 CH3 -> PA10
#define SEAL_MOTOR_IN1_TIM		(htim1)
#define SEAL_MOTOR_IN1_PWM_CH	(TIM_CHANNEL_3)

// Full scale of the IN1 duty cycle == htim1.Init.Period + 1 (MX_TIM1_Init: Period = 9999)
#define SEAL_MOTOR_PWM_MAX		(10000)

/* tim.c configures TIM1 CH3 as PWM1 with TIM_OCPOLARITY_LOW, i.e. the compare
 * value counts the time PA10 stays LOW:
 *     IN1 high duty = (SEAL_MOTOR_PWM_MAX - CCR) / SEAL_MOTOR_PWM_MAX
 * The H-bridge however drives forward on IN1 = 1, so seal_control.c compensates
 * for that inversion. Set this to 0 if tim.c is ever regenerated with
 * TIM_OCPOLARITY_HIGH - no other code needs to change. */
#define SEAL_MOTOR_PWM_ACTIVE_LOW	(1)


// Cooler: cooler 1: TIM12 CH2 cooler 2: TIM12 CH1
#define COOLER_1_TIM		(htim12)
#define COOLER_1_PWM_CH		(TIM_CHANNEL_2)

#define COOLER_2_TIM		(htim12)
#define COOLER_2_PWM_CH		(TIM_CHANNEL_1)


// Cooler Fan: all TIM4, fan1~fan4: CH4~CH1
#define FAN_1_TIM			(htim4)
#define FAN_1_PWM_CH		(TIM_CHANNEL_4)

#define FAN_2_TIM			(htim4)
#define FAN_2_PWM_CH		(TIM_CHANNEL_3)

#define FAN_3_TIM			(htim4)
#define FAN_3_PWM_CH		(TIM_CHANNEL_2)

#define FAN_4_TIM			(htim4)
#define FAN_4_PWM_CH		(TIM_CHANNEL_1)


// Power Fan: TIM3 CH1
#define POWER_FAN_TIM		(htim3)
#define POWER_FAN_PWM_CH	(TIM_CHANNEL_1)


#endif /* INC_BSP_TIM_H_ */
