/*
 * app.h
 *
 *  Created on: 2026年8月31日
 *      Author: ranfa
 *
 *  超循环的应用层。所有周期性动作都由 App_Tick() 用 HAL_GetTick() 差值驱动，
 *  没有 RTOS、没有 osDelay。
 */

#ifndef INC_APP_H_
#define INC_APP_H_

#include <stdint.h>

void App_Init(void);
void App_Tick(uint32_t now);

#endif /* INC_APP_H_ */
