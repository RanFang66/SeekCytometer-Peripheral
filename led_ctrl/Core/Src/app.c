/*
 * app.c
 *
 *  Created on: 2026年8月31日
 *      Author: ranfa
 */

#include "app.h"
#include "board_config.h"
#include "bsp_gpio.h"
#include "bsp_tim.h"
#include "tlc5957.h"
#include "debug_shell.h"
#include "shell_commands.h"

/* Heartbeat toggle period. 500 ms toggle is a 1 Hz square wave, so the scope
 * should read a 1.000 s period. SysTick's reload value is derived by the HAL
 * from SystemCoreClock, so this period doubles as a clock-tree check: a
 * misconfigured PLL shows up here as a proportional error. */
#define HEARTBEAT_TOGGLE_MS		(500U)

static uint32_t heartbeatLast;

void App_Init(void)
{
#ifdef BSP_HEARTBEAT_NEEDS_INIT
	/* Fallback: PA9 is in the .ioc, but this generated code predates that edit,
	 * so MX_GPIO_Init() does not configure it yet. Once CubeMX regenerates,
	 * main.h defines heartbeat_Pin, BSP_HEARTBEAT_NEEDS_INIT goes away and this
	 * block compiles out on its own - nothing to delete later. */
	GPIO_InitTypeDef gi = {0};
	__HAL_RCC_GPIOA_CLK_ENABLE();
	HAL_GPIO_WritePin(HEARTBEAT_PORT, HEARTBEAT_PIN, GPIO_PIN_RESET);
	gi.Pin   = HEARTBEAT_PIN;
	gi.Mode  = GPIO_MODE_OUTPUT_PP;
	gi.Pull  = GPIO_NOPULL;
	gi.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(HEARTBEAT_PORT, &gi);
#endif

	/* GCLK must run continuously: the TLC5957 grayscale counter only advances
	 * while it does, and stopping it blanks the strip. This call moves into
	 * TLC5957_Init() at M3, next to the FC write and the GS clear. */
	HAL_TIM_PWM_Start(&TLC_GCLK_TIM, TLC_GCLK_PWM_CH);

	/* GCLK first, then the driver: TLC5957_Init() ends with a LATGS, and the
	 * device only acts on it while the grayscale clock is running. */
	TLC5957_Init();

	heartbeatLast = HAL_GetTick();

#if USART1_ROLE == USART1_ROLE_SHELL
	Shell_Init();
	registerDebugCommands();
	Shell_Start();
#endif
}

void App_Tick(uint32_t now)
{
#if USART1_ROLE == USART1_ROLE_SHELL
	Shell_Poll();
#endif

	if ((uint32_t)(now - heartbeatLast) >= HEARTBEAT_TOGGLE_MS) {
		heartbeatLast = now;
		HAL_GPIO_TogglePin(HEARTBEAT_PORT, HEARTBEAT_PIN);
	}
}
