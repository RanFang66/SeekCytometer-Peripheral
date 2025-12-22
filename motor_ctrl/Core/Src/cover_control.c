/*
 * cover_control.c
 *
 *  Created on: 2025年11月28日
 *      Author: ranfa
 */


#include "cover_control.h"
#include "bsp_tim.h"
#include "bsp_gpio.h"
#include "cmsis_os2.h"
#include "debug_shell.h"

static DCMotor_t 			coverMotor;
static const PosSensor_t	pClosed = {.gpio=COVER_CLOSED_GPIO, .pin=COVER_CLOSED_PIN};
static const PosSensor_t	pAlmostClosed = {.gpio=COVER_ALMOST_CLOSED_GPIO, .pin=COVER_ALMOST_CLOSED_PIN};
static const PosSensor_t	pAlmostOpened = {.gpio=COVER_ALMOST_OPENED_GPIO, .pin=COVER_ALMOST_OPENED_PIN};
static const PosSensor_t	pOpened = {.gpio=COVER_OPENED_GPIO, .pin=COVER_OPENED_PIN};

static CoverCtrl_Ctx_t coverCtrl;
static osThreadId_t coverThread = NULL;
static osMessageQueueId_t coverCmdQueue = NULL;



static CoverPos_t Cover_UpdatePos()
{
	GPIO_PinState sClosed, sAlmostClosed, sAlmostOpened, sOpened;
	sClosed = HAL_GPIO_ReadPin(pClosed.gpio, pClosed.pin);
	sAlmostClosed = HAL_GPIO_ReadPin(pAlmostClosed.gpio, pAlmostClosed.pin);
	sAlmostOpened = HAL_GPIO_ReadPin(pAlmostOpened.gpio, pAlmostOpened.pin);
	sOpened = HAL_GPIO_ReadPin(pOpened.gpio, pOpened.pin);

	uint8_t pos = (sOpened << 3) | (sAlmostOpened << 2) | (sAlmostClosed << 1) | sClosed;

	switch (pos) {
	case 0x00:
		return COVER_POS_MIDDLE;
	case 0x01:
	case 0x03:
		return COVER_POS_CLOSED;
	case 0x02:
		return COVER_POS_ALMOST_CLOSED;
	case 0x04:
		return COVER_POS_ALMOST_OPENED;
	case 0x08:
	case 0x0C:
		return COVER_POS_OPENED;
	default:
		return COVER_POS_UNDEFINED;
	}
}


void CoverControl_Init()
{
	coverCtrl.motor = &coverMotor;
//	coverCtrl.pClosed = &pClosed;
//	coverCtrl.pAlmostClosed = &pAlmostClosed;
//	coverCtrl.pAlmostOpened = &pAlmostOpened;
//	coverCtrl.pOpened = &pOpened;
	DCMotor_Init(&coverMotor, &COVER_MOTOR_TIM, COVER_MOTOR_PWM_CH, COVER_MOTOR_EN_GPIO, COVER_MOTOR_EN_PIN, COVER_MOTOR_DIR_GPIO, COVER_MOTOR_DIR_PIN);
	coverCtrl.status = COVER_IDLE;
	coverCtrl.pos = Cover_UpdatePos();
	coverCtrl.lastPos = coverCtrl.pos;
	coverCtrl.accVel = DEFAULT_ACC_VEL;
	coverCtrl.normalVel = DEFAULT_NORMAL_VEL;
	coverCtrl.descVel = DEFAULT_DESC_VEL;
}





#define CoverMotor_OpenNormal()   DCMotor_RunCW(coverCtrl.motor, coverCtrl.normalVel)
#define CoverMotor_OpenDesc()		DCMotor_RunCW(coverCtrl.motor, coverCtrl.descVel)


#define CoverMotor_CloseNormal()	DCMotor_RunCCW(coverCtrl.motor, coverCtrl.normalVel)
#define CoverMotor_CloseDesc()	DCMotor_RunCCW(coverCtrl.motor, coverCtrl.descVel)

#define CoverMotor_Stop() 		DCMotor_DisablePower(coverCtrl.motor)

static void Cover_StartOpen()
{
	switch (coverCtrl.pos) {
	case COVER_POS_CLOSED:
	case COVER_POS_ALMOST_CLOSED:
	case COVER_POS_MIDDLE:
		CoverMotor_OpenNormal();
		break;
	case COVER_POS_ALMOST_OPENED:
		CoverMotor_OpenDesc();
		break;
	default:
		CoverMotor_Stop();
		break;
	}
}

static void Cover_StartClose()
{
	switch (coverCtrl.pos) {
	case COVER_POS_OPENED:
	case COVER_POS_ALMOST_OPENED:
	case COVER_POS_MIDDLE:
		CoverMotor_CloseNormal();
		break;
	case COVER_POS_ALMOST_CLOSED:
		CoverMotor_CloseDesc();
		break;
	default:
		CoverMotor_Stop();
		break;
	}
}



static void CoverControl_Task(void *arg)
{
	CoverCmd_t cmd;
	for (;;) {
		// Update position
		coverCtrl.lastPos = coverCtrl.pos;
		coverCtrl.pos = Cover_UpdatePos();

		// Handle position changed
		switch (coverCtrl.status) {
		case COVER_IDLE:
			break;
		case COVER_OPENING:
			if (coverCtrl.lastPos == COVER_POS_MIDDLE && coverCtrl.pos == COVER_POS_ALMOST_OPENED) {
				CoverMotor_OpenDesc();
			} else if (coverCtrl.lastPos == COVER_POS_ALMOST_OPENED && coverCtrl.pos == COVER_POS_OPENED) {
				CoverMotor_Stop();
				coverCtrl.status = COVER_IDLE;
			}
			break;
		case COVER_CLOSING:
			if (coverCtrl.lastPos == COVER_POS_MIDDLE && coverCtrl.pos == COVER_POS_ALMOST_CLOSED) {
				CoverMotor_CloseDesc();
			} else if (coverCtrl.lastPos == COVER_POS_ALMOST_CLOSED && coverCtrl.pos == COVER_POS_CLOSED) {
				CoverMotor_Stop();
				coverCtrl.status = COVER_IDLE;
			}
			break;
		case COVER_ERROR:
		default:
			break;
		}

		// Handle command
		if (osMessageQueueGet(coverCmdQueue, &cmd, NULL, 50) == osOK) {
			if (cmd == COVER_CMD_OPEN && coverCtrl.pos != COVER_POS_OPENED && coverCtrl.status != COVER_OPENING) {
				coverCtrl.status = COVER_OPENING;
				Cover_StartOpen();
			} else if (cmd == COVER_CMD_CLOSE && coverCtrl.pos != COVER_POS_CLOSED && coverCtrl.status != COVER_CLOSING) {
				coverCtrl.status = COVER_CLOSING;
				Cover_StartClose();
			} else if (cmd == COVER_CMD_STOP && coverCtrl.status != COVER_IDLE) {
				coverCtrl.status = COVER_IDLE;
				CoverMotor_Stop();
			}
		}
	}
}

void CoverControl_StartTask()
{
	coverCmdQueue = osMessageQueueNew(COVER_CMD_QUEUE_SIZE, sizeof(CoverCmd_t), NULL);
	if (coverCmdQueue == NULL) {
		LOG_ERROR("Create cover command queue FAILED!");
	}

	const osThreadAttr_t taskAttr = {.name = "CoverControl", .priority=osPriorityNormal, .stack_size=256};
	coverThread = osThreadNew(CoverControl_Task, NULL, &taskAttr);

	if (coverThread == NULL) {
		LOG_ERROR("Create cover control thread FAILED!");
	} else {
		LOG_INFO("Create cover control thread OK");
	}

}


CoverStatus_t Cover_GetStatus()
{
	return coverCtrl.status;
}

CoverPos_t Cover_GetPos()
{
	return coverCtrl.pos;
}

void Cover_Open()
{
	CoverCmd_t cmd = COVER_CMD_OPEN;
	osStatus_t st = osMessageQueuePut(coverCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send cover open command FAILED!");
	}
}

void Cover_Close()
{
	CoverCmd_t cmd = COVER_CMD_CLOSE;
	osStatus_t st = osMessageQueuePut(coverCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send cover close command FAILED!");
	}
}

void Cover_Stop()
{
	CoverCmd_t cmd = COVER_CMD_STOP;
	osStatus_t st = osMessageQueuePut(coverCmdQueue, &cmd, 0, 100);
	if (st != osOK) {
		LOG_WARNING("Send cover stop command FAILED!");
	}
}

void Cover_SetAccSpeed(uint16_t vel)
{
	coverCtrl.accVel = vel;
}


void Cover_SetNormalSpeed(uint16_t vel)
{
	coverCtrl.normalVel = vel;
}

void Cover_SetDescSpeed(uint16_t vel)
{
	coverCtrl.descVel = vel;
}

