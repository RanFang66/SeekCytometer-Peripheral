/*
 * swpwm.h
 *
 *  Created on: 2026年6月1日
 *      Author: ranfa
 *
 *  Software (timer-interrupt driven) PWM on a plain GPIO output pin.
 *  Designed for pins that have no timer PWM alternate function (e.g. PD11).
 *
 *  A free basic timer (TIM7) is used as the time base. Each PWM period only
 *  produces two update interrupts (one for the rising edge, one for the
 *  falling edge), so the CPU load stays low even at a few kHz.
 *
 *  Speed control is done through the duty cycle (0..100 %).
 */

#ifndef INC_SWPWM_H_
#define INC_SWPWM_H_

#include "tim.h"
#include "gpio.h"

typedef struct {
	GPIO_TypeDef		*port;			// output GPIO port
	uint16_t			pin;			// output GPIO pin
	TIM_HandleTypeDef	*htim;			// time base timer (counter clock = 1 MHz)
	uint16_t			periodCounts;	// total counter ticks of one PWM period
	volatile uint16_t	onCounts;		// high-phase ticks
	volatile uint16_t	offCounts;		// low-phase ticks
	volatile uint8_t	duty;			// current duty cycle 0..100 %
	volatile uint8_t	phaseHigh;		// 1 = output currently high, 0 = low
	volatile uint8_t	running;		// 1 = timer interrupt active (PWM region)
} SwPwm_t;

/*
 * Bind a software PWM channel to a GPIO pin and a 1 MHz time base timer.
 * pwmFreqHz is the carrier frequency (the duty cycle does the speed control).
 * The timer must already be initialised (MX_TIMx_Init) with a 1 MHz tick.
 * The output starts stopped (pin low).
 */
void SwPwm_Init(SwPwm_t *p, GPIO_TypeDef *port, uint16_t pin,
		TIM_HandleTypeDef *htim, uint32_t pwmFreqHz);

/* Set duty cycle (0..100). 0 -> pin low & timer off, 100 -> pin high & timer off. */
void SwPwm_SetDuty(SwPwm_t *p, uint8_t duty);

/* Start the PWM output at the given duty cycle. */
void SwPwm_Start(SwPwm_t *p, uint8_t duty);

/* Stop the PWM output (pin driven low, timer stopped). */
void SwPwm_Stop(SwPwm_t *p);

/* Current duty cycle (0..100). */
uint8_t SwPwm_GetDuty(SwPwm_t *p);

/*
 * Update-interrupt service routine for the registered instance.
 * Call this from the timer's IRQ handler (e.g. TIM7_IRQHandler).
 */
void SwPwm_HandleIRQ(void);

#endif /* INC_SWPWM_H_ */
