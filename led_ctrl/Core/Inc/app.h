/*
 * app.h
 *
 *  Created on: 2026年8月31日
 *      Author: ranfa
 *
 *  Superloop application layer. Everything periodic is driven by App_Tick()
 *  through HAL_GetTick() deltas. No RTOS, no osDelay.
 */

#ifndef INC_APP_H_
#define INC_APP_H_

#include <stdint.h>

void App_Init(void);
void App_Tick(uint32_t now);

#endif /* INC_APP_H_ */
