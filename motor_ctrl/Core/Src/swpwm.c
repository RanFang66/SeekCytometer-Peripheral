/*
 * swpwm.c
 *
 *  Created on: 2026年6月1日
 *      Author: ranfa
 */

#include "swpwm.h"
#include "debug_shell.h"

/*
 * Only a single software PWM instance is needed (the DC churn motor on PD11),
 * so the IRQ handler dispatches to one registered instance. The time base
 * timer counter clock is assumed to be 1 MHz (1 tick = 1 us), configured by
 * MX_TIM7_Init().
 */
static SwPwm_t *s_inst = NULL;

#define SWPWM_TIMER_CLOCK_HZ	(1000000UL)		/* 1 MHz counter clock */

/* Reprogram on/off tick counts from a duty cycle (1..99). Caller guarantees range. */
static void SwPwm_ComputeCounts(SwPwm_t *p, uint8_t duty)
{
	uint32_t on = ((uint32_t)p->periodCounts * duty + 50U) / 100U;	/* rounded */
	if (on < 1U) {
		on = 1U;
	}
	if (on > (uint32_t)(p->periodCounts - 1U)) {
		on = p->periodCounts - 1U;
	}
	p->onCounts = (uint16_t)on;
	p->offCounts = (uint16_t)(p->periodCounts - on);
}

void SwPwm_Init(SwPwm_t *p, GPIO_TypeDef *port, uint16_t pin,
		TIM_HandleTypeDef *htim, uint32_t pwmFreqHz)
{
	if (p == NULL || port == NULL || htim == NULL || pwmFreqHz == 0U) {
		LOG_ERROR("Initialize SwPwm Failed! Invalid parameter input");
		return;
	}

	p->port = port;
	p->pin = pin;
	p->htim = htim;
	p->periodCounts = (uint16_t)(SWPWM_TIMER_CLOCK_HZ / pwmFreqHz);
	if (p->periodCounts < 2U) {
		p->periodCounts = 2U;	/* keep at least 1 tick per phase */
	}
	p->onCounts = 0U;
	p->offCounts = p->periodCounts;
	p->duty = 0U;
	p->phaseHigh = 0U;
	p->running = 0U;

	/* Output low, timer stopped */
	HAL_TIM_Base_Stop_IT(htim);
	HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);

	s_inst = p;
}

void SwPwm_SetDuty(SwPwm_t *p, uint8_t duty)
{
	if (p == NULL) {
		return;
	}
	if (duty > 100U) {
		duty = 100U;
	}

	if (duty == 0U) {
		SwPwm_Stop(p);
		return;
	}

	if (duty >= 100U) {
		/* Full on: no switching needed, drive the pin high continuously */
		HAL_NVIC_DisableIRQ(TIM7_IRQn);
		HAL_TIM_Base_Stop_IT(p->htim);
		p->running = 0U;
		p->phaseHigh = 1U;
		HAL_GPIO_WritePin(p->port, p->pin, GPIO_PIN_SET);
		p->duty = 100U;
		HAL_NVIC_EnableIRQ(TIM7_IRQn);
		return;
	}

	/* 1..99 %: switching PWM */
	HAL_NVIC_DisableIRQ(TIM7_IRQn);
	SwPwm_ComputeCounts(p, duty);
	p->duty = duty;

	if (!p->running) {
		/* (Re)start: begin with the high phase */
		p->phaseHigh = 1U;
		HAL_GPIO_WritePin(p->port, p->pin, GPIO_PIN_SET);
		__HAL_TIM_SET_AUTORELOAD(p->htim, p->onCounts - 1U);
		__HAL_TIM_SET_COUNTER(p->htim, 0U);
		__HAL_TIM_CLEAR_FLAG(p->htim, TIM_FLAG_UPDATE);
		p->running = 1U;
		HAL_TIM_Base_Start_IT(p->htim);
	}
	/* If already running, the ISR picks up the new on/off counts at the
	 * next phase boundary; nothing else to do here. */
	HAL_NVIC_EnableIRQ(TIM7_IRQn);
}

void SwPwm_Start(SwPwm_t *p, uint8_t duty)
{
	SwPwm_SetDuty(p, duty);
}

void SwPwm_Stop(SwPwm_t *p)
{
	if (p == NULL) {
		return;
	}
	HAL_NVIC_DisableIRQ(TIM7_IRQn);
	HAL_TIM_Base_Stop_IT(p->htim);
	p->running = 0U;
	p->phaseHigh = 0U;
	HAL_GPIO_WritePin(p->port, p->pin, GPIO_PIN_RESET);
	p->duty = 0U;
	HAL_NVIC_EnableIRQ(TIM7_IRQn);
}

uint8_t SwPwm_GetDuty(SwPwm_t *p)
{
	return (p != NULL) ? p->duty : 0U;
}

void SwPwm_HandleIRQ(void)
{
	SwPwm_t *p = s_inst;
	if (p == NULL) {
		return;
	}

	TIM_TypeDef *TIMx = p->htim->Instance;
	if ((TIMx->SR & TIM_SR_UIF) == 0U) {
		return;
	}
	TIMx->SR = ~TIM_SR_UIF;	/* clear update flag (write-zero-to-clear) */

	if (p->phaseHigh) {
		/* end of high phase -> go low for offCounts ticks */
		HAL_GPIO_WritePin(p->port, p->pin, GPIO_PIN_RESET);
		p->phaseHigh = 0U;
		TIMx->ARR = p->offCounts - 1U;
	} else {
		/* end of low phase -> go high for onCounts ticks */
		HAL_GPIO_WritePin(p->port, p->pin, GPIO_PIN_SET);
		p->phaseHigh = 1U;
		TIMx->ARR = p->onCounts - 1U;
	}
}
