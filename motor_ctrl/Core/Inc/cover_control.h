/*
 * cover_control.h
 *
 *  Created on: 2025年11月28日
 *      Author: ranfa
 */

#ifndef INC_COVER_CONTROL_H_
#define INC_COVER_CONTROL_H_

/* The hardware revision removed the cover mechanism and re-assigned its
 * direction pin (PA11) to the seal motor H-bridge IN2, so the cover module must
 * stay off: it would fight the seal driver for that pin.
 *
 * Setting this to 1 re-enables the whole module (implementation, task, Modbus
 * command dispatch and the "cover" shell command) - but PA11 has to be sorted
 * out first, see SEAL_MOTOR_IN2_* in bsp_gpio.h.
 *
 * The types and prototypes below stay visible on purpose: a call site that is
 * missed by the guards then fails at link time instead of silently building. */
#ifndef ENABLE_COVER_CTRL
#define ENABLE_COVER_CTRL	0
#endif

#include "dc_motor.h"

typedef enum {
	COVER_IDLE = 0,
	COVER_OPENING,
	COVER_CLOSING,
	COVER_ERROR,
} CoverStatus_t;

typedef enum {
	COVER_POS_CLOSED = 0,
	COVER_POS_ALMOST_CLOSED,
	COVER_POS_MIDDLE,
	COVER_POS_ALMOST_OPENED,
	COVER_POS_OPENED,
	COVER_POS_UNDEFINED,
} CoverPos_t;


#define DEFAULT_ACC_VEL 		(6000)
#define DEFAULT_NORMAL_VEL		(5000)
#define DEFAULT_DESC_VEL		(3500)

#define COVER_CMD_QUEUE_SIZE 	8
typedef struct {
	GPIO_TypeDef 		*gpio;
	uint16_t			pin;
} PosSensor_t;


//PosSensor_t		*pClosed;
//PosSensor_t		*pAlmostClosed;
//PosSensor_t		*pAlmostOpened;
//PosSensor_t		*pOpened;

typedef struct {
	DCMotor_t 		*motor;
	CoverStatus_t 	status;
	CoverPos_t		pos;
	CoverPos_t		lastPos;
	uint16_t 		accVel;
	uint16_t		normalVel;
	uint16_t 		descVel;
} CoverCtrl_Ctx_t;

typedef enum {
	COVER_CMD_STOP = 0,
	COVER_CMD_OPEN,
	COVER_CMD_CLOSE,
} CoverCmd_t;

void CoverControl_Init();
void Cover_Open();
void Cover_Close();
void Cover_Stop();
void Cover_SetAccSpeed(uint16_t vel);
void Cover_SetNormalSpeed(uint16_t vel);
void Cover_SetDescSpeed(uint16_t vel);

CoverStatus_t Cover_GetStatus();
CoverPos_t Cover_GetPos();
void CoverControl_StartTask();


#endif /* INC_COVER_CONTROL_H_ */
