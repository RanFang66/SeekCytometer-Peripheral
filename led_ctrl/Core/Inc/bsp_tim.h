/*
 * bsp_tim.h
 *
 *  Created on: 2026年8月31日
 *      Author: ranfa
 */

#ifndef INC_BSP_TIM_H_
#define INC_BSP_TIM_H_

#include "tim.h"

/* TLC5957 灰度时钟 GCLK：TIM14 CH1 -> PB1 */
#define TLC_GCLK_TIM		(htim14)
#define TLC_GCLK_PWM_CH		(TIM_CHANNEL_1)

/* MX_TIM14_Init(): PSC = 0, Period(ARR) = 3, Pulse(CCR) = 2, PWM1
 *     f = 48 MHz / (PSC+1) / (ARR+1) = 12 MHz，占空 CCR/(ARR+1) = 50%
 * 改 tim.c 里那两个值的话，这里两个常量必须跟着改 —— TLC5957 的显示刷新率
 * 直接由它推出来，图案引擎的抗闪判断依赖这个数。 */
#define TLC_GCLK_HZ			(12000000UL)

/* TLC5957 是 16 bit 灰度，一个显示周期需要 65536 个 GCLK。
 * 12 MHz / 65536 ≈ 183 Hz，远高于人眼闪烁阈值。 */
#define TLC_GS_CLOCKS		(65536UL)
#define TLC_REFRESH_HZ		(TLC_GCLK_HZ / TLC_GS_CLOCKS)

#endif /* INC_BSP_TIM_H_ */
