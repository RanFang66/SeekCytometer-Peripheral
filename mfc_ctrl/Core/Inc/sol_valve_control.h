/*
 * sloenoid_pwm.h
 *
 *  Created on: Oct 24, 2025
 *      Author: ranfa
 */

#ifndef INC_SOL_VALVE_CONTROL_H_
#define INC_SOL_VALVE_CONTROL_H_


#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 阀门数量固定为 5
#define SOL_VALVE_COUNT 		5
#define MIN_BOOST_TIME_MS 		80

#define SOL_CTRL_CLOSE 		0
#define SOL_CTRL_OPEN		1
// 状态机
typedef enum {
    SOL_STATE_OFF = 0,   // 0% 占空
    SOL_STATE_ON       // 保持（默认 50%）
} sol_state_t;

// 每路阀的绑定信息
typedef struct {
	GPIO_TypeDef 	  	*gpioEn;
	uint16_t			pinEn;

    sol_state_t        state;
} sol_ch_t;


/**
 * @brief 初始化 5 路电磁阀 PWM 驱动
 */
HAL_StatusTypeDef SOL_Init();

/** 开阀：100% 占空并在 boost_ms 后自动降到保持占空 */
HAL_StatusTypeDef SOL_Open(uint8_t id);

/** 关阀：0% 占空、停止提升计时器 */
HAL_StatusTypeDef SOL_Close(uint8_t id);

/** 全开（分别应用提升→保持） */
HAL_StatusTypeDef SOL_OpenAll();

/** 全关 */
HAL_StatusTypeDef SOL_CloseAll();


/** 获取单路当前状态 */
sol_state_t SOL_GetState(uint8_t id);


#ifdef __cplusplus
}
#endif

#endif /* INC_SOL_VALVE_CONTROL_H_ */
