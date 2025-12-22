/*
 * led_control.h
 *
 *  Created on: 2025年12月9日
 *      Author: ranfa
 */

#ifndef INC_LED_CONTROL_H_
#define INC_LED_CONTROL_H_

#include "mcp4728.h"

#define LED_OFF 	0
#define LED_ON 		1


#define LED_CMD_SWITCH_OFF		0
#define LED_CMD_SWITCH_ON			1
#define LED_CMD_SET_INTENSITY		2


typedef struct {
	MCP4728_Channel_t 	channel;
	GPIO_TypeDef		*gpioEn;
	uint16_t			pinEn;
	uint8_t 			status;
	uint16_t 			intensity;
} LedCtrlCtx_t;

void LED_Init();
void LED_SwitchOff();
void LED_SwitchOn(uint16_t val);
void LED_SetIntensity(uint16_t val);
uint8_t LED_GetStatus();
uint16_t LED_GetIntensity();

#endif /* INC_LED_CONTROL_H_ */
