/*
 * bsp_gpio.h
 *
 *  Created on: 2026年8月31日
 *      Author: ranfa
 *
 *  角色 -> 引脚映射。所有引脚都在 led_ctrl.ioc 里配置，这里只做语义化改名，
 *  这样上层代码不依赖 CubeMX 的标签拼写，重新生成后只需要改这一个文件。
 */

#ifndef INC_BSP_GPIO_H_
#define INC_BSP_GPIO_H_

#include "main.h"

/* ---- TLC5957 串行接口（GPIO 软件模拟，见详细设计 §6.1）---- */
#define TLC_SCLK_PORT		TLC_SCLK_GPIO_Port		// PA5
#define TLC_SCLK_PIN		TLC_SCLK_Pin
#define TLC_LAT_PORT		TLC_LAT_GPIO_Port		// PA6
#define TLC_LAT_PIN			TLC_LAT_Pin
#define TLC_SIN_PORT		TLC_SIN_GPIO_Port		// PA7
#define TLC_SIN_PIN			TLC_SIN_Pin

/* ---- 舱门电磁锁 ---- */
#define LOCK_EN_PORT		lock_en_GPIO_Port		// PA4，外部 R7 10K 下拉
#define LOCK_EN_PIN			lock_en_Pin
#define LOCK_FB_PORT		lock_feedback_GPIO_Port	// PA1，经 U2 缓冲的干接点
#define LOCK_FB_PIN			lock_feedback_Pin

/* ---- 备用干接点输入（P9 座子）---- */
#define AUX_IN_PORT			aux_in_GPIO_Port		// PA10
#define AUX_IN_PIN			aux_in_Pin

/* ---- 心跳测试点 ----
 * PA9 在原理图上悬空，是这块板上唯一的示波器观察点。切到 Debug_Modbus 构型后
 * 没有 shell，它就是判断主循环是否还活着的唯一手段。
 *
 * PA9 已加进 led_ctrl.ioc，但生成代码是在那之前做的，所以 main.h 里暂时还没有
 * heartbeat_Pin。下面的兜底定义 + app.c 里的兜底初始化会在 CubeMX 下次重新生成
 * 之后自动失效，不需要回头清理。 */
#ifndef heartbeat_Pin
#define heartbeat_Pin			GPIO_PIN_9
#define heartbeat_GPIO_Port		GPIOA
#define BSP_HEARTBEAT_NEEDS_INIT	1				/* 见 App_Init() */
#endif

#define HEARTBEAT_PORT		heartbeat_GPIO_Port
#define HEARTBEAT_PIN		heartbeat_Pin

#endif /* INC_BSP_GPIO_H_ */
