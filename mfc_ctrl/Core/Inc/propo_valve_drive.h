/*
 * propo_valve_drive.h
 *
 *  Created on: Oct 22, 2025
 *      Author: ranfa
 */

#ifndef INC_PROPO_VALVE_DRIVE_H_
#define INC_PROPO_VALVE_DRIVE_H_

#include "dac8568.h"

#define PROPO_VALVE_NUM 	(5)

typedef struct {
	dac8568_channel_t  	dacChannel;
	uint16_t 			shadowValue;
} PropoValveDrive_t;

typedef enum {
	PROPO_VALVE_0 = 0,
	PROPO_VALVE_1,
	PROPO_VALVE_2,
	PROPO_VALVE_3,
	PROPO_VALVE_4,
} ValveID_t;


#define PROPO_CTRL_CLOSE 	0
#define PROPO_CTRL_OPEN 	1

HAL_StatusTypeDef  PropoValveDrive_Init();
HAL_StatusTypeDef  PropoValveDrive_Set(ValveID_t id, uint16_t value);
HAL_StatusTypeDef  PropoValveDrive_SetAndUpdate(ValveID_t id, uint16_t value);
HAL_StatusTypeDef  PropoValveDrive_SetAllAndUpdate(const uint16_t values[PROPO_VALVE_NUM]);
HAL_StatusTypeDef  PropoValveDrive_UpdateAll();

HAL_StatusTypeDef  PropoValveDrive_Close(ValveID_t id);
HAL_StatusTypeDef  PropoValveDrive_CloseAll();
uint16_t		   PropoValveDrive_GetValue(ValveID_t id);


#endif /* INC_PROPO_VALVE_DRIVE_H_ */
