/*
 * laser_control.h
 *
 *  Created on: 2025年12月9日
 *      Author: ranfa
 */

#ifndef INC_LASER_CONTROL_H_
#define INC_LASER_CONTROL_H_


#include "mcp4728.h"

#define LASER_NUM 		(2)

#define LASER_OFF		(0)
#define LASER_ON		(1)

#define LASER_CMD_SWITCH_OFF		0
#define LASER_CMD_SWITCH_ON			1
#define LASER_CMD_SET_INTENSITY		2


typedef enum {
	LASER_1 = 0,
	LASER_2,
	LASER_ALL,
} LaserIndex_t;

typedef struct {
	MCP4728_Channel_t 	channel;
	GPIO_TypeDef		*gpioEn;
	uint16_t			pinEn;
	uint8_t				status;
	uint16_t			intensity;
} LaserCtrlCtx_t;


void Laser_Init();
void Laser_SwitchOff(LaserIndex_t id);
void Laser_SwitchOn(LaserIndex_t id, uint16_t val);
void Laser_SetIntensity(LaserIndex_t id, uint16_t val);
uint8_t Laser_GetStatus(LaserIndex_t id);
uint16_t Laser_GetIntensity(LaserIndex_t id);

#endif /* INC_LASER_CONTROL_H_ */
