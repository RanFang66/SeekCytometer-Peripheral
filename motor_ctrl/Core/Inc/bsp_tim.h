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


// cover motor: TIM2 CH1
#define COVER_MOTOR_TIM 	(htim2)
#define COVER_MOTOR_PWM_CH	(TIM_CHANNEL_1)

// seal motor: TIM1 CH3
#define SEAL_MOTOR_TIM		(htim1)
#define SEAL_MOTOR_PWM_CH	(TIM_CHANNEL_3)


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
