/*
 * bsp_tim.h
 *
 *  Created on: 2026年8月31日
 *      Author: ranfa
 */

#ifndef INC_BSP_TIM_H_
#define INC_BSP_TIM_H_

#include "tim.h"

/* TLC5957 grayscale clock GCLK: TIM14 CH1 -> PB1 */
#define TLC_GCLK_TIM		(htim14)
#define TLC_GCLK_PWM_CH		(TIM_CHANNEL_1)

/* MX_TIM14_Init(): PSC = 0, Period(ARR) = 3, Pulse(CCR) = 2, PWM1
 *     f = 48 MHz / (PSC+1) / (ARR+1) = 12 MHz, duty = CCR/(ARR+1) = 50%
 * If those two values change in tim.c, the constants below must follow: the
 * TLC5957 refresh rate is derived from them, and the pattern engine's
 * flicker-free assumption rests on that number. */
#define TLC_GCLK_HZ			(12000000UL)

/* TLC5957 grayscale is 16 bit, so one display period takes 65536 GCLK edges.
 * 12 MHz / 65536 = 183 Hz, comfortably above the flicker threshold. */
#define TLC_GS_CLOCKS		(65536UL)
#define TLC_REFRESH_HZ		(TLC_GCLK_HZ / TLC_GS_CLOCKS)

#endif /* INC_BSP_TIM_H_ */
