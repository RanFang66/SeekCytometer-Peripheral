/*
 * modbus_local_table.c
 *
 *  Created on: 2025年11月27日
 *      Author: ranfa
 */

#include "modbus_slave.h"
#include "hsc_spi.h"
#include "propo_valve_drive.h"
#include "sol_valve_control.h"
#include "press_control.h"


typedef union {
	uint16_t word;
	struct BITS{
		uint16_t pressCtrl 	: 	1;
		uint16_t soleCtrl	:   1;
		uint16_t propoCtrl  :   1;
		uint16_t reserved	:   13;
	} bits;
} CtrlWord_t;


static CtrlWord_t ctrlWord = {.word = 0};
static CtrlWord_t lastCtrlWord = {.word = 0};
static uint16_t pressCmdCh;
static uint16_t propoCmdCh;
static uint16_t soleCmdCh;
static uint16_t pressCmd;
static uint16_t propoCmd;
static uint16_t soleCmd;
static uint16_t pressTargetSet[PRESS_CTRL_CH_NUM];
static uint16_t propoCmdData;

static uint16_t soleCtrlStatus = 0;
static uint16_t pressMeasured[PRESS_CTRL_CH_NUM];
static uint16_t propoValveValue[PROPO_VALVE_NUM];
static uint16_t inputPressx10 = 0;
static uint16_t pressCtrlStatus = 0;
static uint16_t pressTarget[PRESS_CTRL_CH_NUM];

static uint16_t kp_x100;
static uint16_t ki_x100;
static uint16_t feedforward;
static uint16_t kp[PRESS_CTRL_CH_NUM];
static uint16_t ki[PRESS_CTRL_CH_NUM];
static uint16_t ff[PRESS_CTRL_CH_NUM];




void MB_Local_RegInit(void)
{
	MB_Slave_DefineReg(0, &ctrlWord.word);

	MB_Slave_DefineReg(8, &pressCmd);
	MB_Slave_DefineReg(9, &pressCmdCh);


	MB_Slave_DefineReg(10, &pressTargetSet[0]);
	MB_Slave_DefineReg(11, &pressTargetSet[1]);
	MB_Slave_DefineReg(12, &pressTargetSet[2]);
	MB_Slave_DefineReg(13, &pressTargetSet[3]);
	MB_Slave_DefineReg(14, &pressTargetSet[4]);


	MB_Slave_DefineReg(16, &soleCmd);
	MB_Slave_DefineReg(17, &soleCmdCh);
	MB_Slave_DefineReg(18, &propoCmd);
	MB_Slave_DefineReg(19, &propoCmdCh);
	MB_Slave_DefineReg(20, &propoCmdData);
	MB_Slave_DefineReg(21, &kp_x100);
	MB_Slave_DefineReg(22, &ki_x100);
	MB_Slave_DefineReg(23, &feedforward);

	MB_Slave_DefineReg(54, &pressCtrlStatus);
	MB_Slave_DefineReg(55, &soleCtrlStatus);
	MB_Slave_DefineReg(56, &inputPressx10);
	MB_Slave_DefineReg(57, &pressMeasured[0]);
	MB_Slave_DefineReg(58, &pressMeasured[1]);
	MB_Slave_DefineReg(59, &pressMeasured[2]);
	MB_Slave_DefineReg(60, &pressMeasured[3]);
	MB_Slave_DefineReg(61, &pressMeasured[4]);
	MB_Slave_DefineReg(62, &propoValveValue[0]);
	MB_Slave_DefineReg(63, &propoValveValue[1]);
	MB_Slave_DefineReg(64, &propoValveValue[2]);
	MB_Slave_DefineReg(65, &propoValveValue[3]);
	MB_Slave_DefineReg(66, &propoValveValue[4]);
	MB_Slave_DefineReg(67, &pressTarget[0]);
	MB_Slave_DefineReg(68, &pressTarget[1]);
	MB_Slave_DefineReg(69, &pressTarget[2]);
	MB_Slave_DefineReg(70, &pressTarget[3]);
	MB_Slave_DefineReg(71, &pressTarget[4]);

	MB_Slave_DefineReg(72, &kp[0]);
	MB_Slave_DefineReg(73, &ki[0]);
	MB_Slave_DefineReg(74, &ff[0]);
	MB_Slave_DefineReg(75, &kp[1]);
	MB_Slave_DefineReg(76, &ki[1]);
	MB_Slave_DefineReg(77, &ff[1]);
	MB_Slave_DefineReg(78, &kp[2]);
	MB_Slave_DefineReg(79, &ki[2]);
	MB_Slave_DefineReg(80, &ff[2]);
	MB_Slave_DefineReg(81, &kp[3]);
	MB_Slave_DefineReg(82, &ki[3]);
	MB_Slave_DefineReg(83, &ff[3]);
	MB_Slave_DefineReg(84, &kp[4]);
	MB_Slave_DefineReg(85, &ki[4]);
	MB_Slave_DefineReg(86, &ff[4]);

}

void MB_UpdateStatus(void)
{
	uint16_t pressStatus[PRESS_CTRL_CH_NUM];
	uint16_t soleStatus[PRESS_CTRL_CH_NUM];
	float sourcePress = 0.0;

	pressCtrlStatus = 0;
	soleCtrlStatus = 0;
	for (uint8_t i = 0; i < PRESS_CTRL_CH_NUM; i++) {
		pressStatus[i] = PressCtrl_GetStatus(i);
		pressCtrlStatus |= pressStatus[i] << 3;
		pressMeasured[i] = PressCtrl_GetLatestPress(i) * 10.0;
		pressTarget[i] = PressCtrl_GetTarget(i);
		soleStatus[i] = SOL_GetState(i);
		soleCtrlStatus |= soleStatus[i] << 3;
		propoValveValue[i] = PropoValveDrive_GetValue(i);
	}
	sourcePress = PressCtrl_GetInputPress();

	inputPressx10 = (uint16_t)(sourcePress * 10.0);
}

void MB_UpdateCtrlParas(void)
{
	for (uint8_t i = 0; i < PRESS_CTRL_CH_NUM; i++) {
		kp[i] = (uint16_t)(PressCtrl_GetKp(i) * 100.0);
		ki[i] = (uint16_t)(PressCtrl_GetKi(i) * 100.0);
		ff[i] = (uint16_t)(PressCtrl_GetFF(i));
	}
}

void MB_CommandParse(void)
{
	if (ctrlWord.word == lastCtrlWord.word) {
		return; 		// No commands
	}
	if (ctrlWord.bits.pressCtrl && !lastCtrlWord.bits.pressCtrl) {
		switch (pressCmd) {
		case PRESS_CTRL_STOP:
			PressCtrl_Stop((uint8_t)pressCmdCh);
			break;

		case PRESS_CTRL_START:
			PressCtrl_Start((uint8_t)pressCmdCh, pressTargetSet);
			break;

		case PRESS_CTRL_SET_TARGET:
			PressCtrl_SetTarget((uint8_t)pressCmdCh, pressTargetSet);
			break;

		case PRESS_CTRL_SET_PI:
			PressCtrl_SetPI((uint8_t)pressCmdCh, kp_x100, ki_x100, feedforward);
			break;

		default:
			break;
		}
	}

	if (ctrlWord.bits.soleCtrl && !lastCtrlWord.bits.soleCtrl) {
		switch (soleCmd) {
		case SOL_CTRL_OPEN:
			SOL_Open((uint8_t)soleCmdCh);
			break;
		case SOL_CTRL_CLOSE:
			SOL_Close((uint8_t)soleCmdCh);
			break;
		default:
			break;
		}
	}

	if (ctrlWord.bits.propoCtrl && !lastCtrlWord.bits.propoCtrl) {
		switch (propoCmd) {
		case PROPO_CTRL_OPEN:
			PropoValveDrive_SetAndUpdate((uint8_t)propoCmdCh, propoCmdData);
			break;
		case PROPO_CTRL_CLOSE:
			PropoValveDrive_Close((uint8_t)propoCmdCh);
			break;
		default:
			break;
		}
	}

	lastCtrlWord.word = ctrlWord.word;
}

