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

/* 心跳翻转周期。500 ms 翻转 = 1 Hz 方波，示波器上周期应为 1.000 s。
 * SysTick 的重装值是 HAL 按 SystemCoreClock 算出来的，所以这个周期对不对，
 * 直接反映时钟树配得对不对 —— PLL 配错的话这里会成比例偏掉。 */
#define HEARTBEAT_TOGGLE_MS		(500U)

static uint32_t heartbeatLast;

void App_Init(void)
{
#ifdef BSP_HEARTBEAT_NEEDS_INIT
	/* 兜底：PA9 已写进 .ioc，但当前这份生成代码早于那次改动，MX_GPIO_Init()
	 * 还没配它。CubeMX 下次重新生成后 main.h 会给出 heartbeat_Pin，
	 * BSP_HEARTBEAT_NEEDS_INIT 随之消失，这段自动编译掉，不用回头删。 */
	GPIO_InitTypeDef gi = {0};
	__HAL_RCC_GPIOA_CLK_ENABLE();
	HAL_GPIO_WritePin(HEARTBEAT_PORT, HEARTBEAT_PIN, GPIO_PIN_RESET);
	gi.Pin   = HEARTBEAT_PIN;
	gi.Mode  = GPIO_MODE_OUTPUT_PP;
	gi.Pull  = GPIO_NOPULL;
	gi.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(HEARTBEAT_PORT, &gi);
#endif

	/* GCLK 必须常开：TLC5957 的灰度计数器靠它走，停了就是全屏灭。
	 * M3 之后这句会挪进 TLC5957_Init()，那里还要接着写 FC 和清 GS。 */
	HAL_TIM_PWM_Start(&TLC_GCLK_TIM, TLC_GCLK_PWM_CH);

	heartbeatLast = HAL_GetTick();
}

void App_Tick(uint32_t now)
{
	if ((uint32_t)(now - heartbeatLast) >= HEARTBEAT_TOGGLE_MS) {
		heartbeatLast = now;
		HAL_GPIO_TogglePin(HEARTBEAT_PORT, HEARTBEAT_PIN);
	}
}
