/*
 * modbus_local_table.c
 *
 *  Created on: 2025年11月27日
 *      Author: ranfa
 */

#include "modbus_slave.h"
#include "sample_control.h"
// sample parameter defined in sample_control.c
extern sample_para_t samplePara[CHANNEL_NUM];

static uint16_t sampleCtrlWord;
static uint16_t lastCtrlWord;
static uint16_t sampleCtrlCmd;
static uint16_t sampleCtrlEnCh;
static uint16_t sampleGainSet[CHANNEL_NUM];
static uint16_t sampleRefSet[CHANNEL_NUM];


void MB_Local_RegInit(void)
{
	MB_Slave_DefineReg(0, &sampleCtrlWord);
	MB_Slave_DefineReg(6, &sampleCtrlCmd);
	MB_Slave_DefineReg(7, &sampleCtrlEnCh);
	MB_Slave_DefineReg(8, &sampleGainSet[0]);
	MB_Slave_DefineReg(9, &sampleGainSet[1]);
	MB_Slave_DefineReg(10, &sampleGainSet[2]);
	MB_Slave_DefineReg(11, &sampleGainSet[3]);
	MB_Slave_DefineReg(12, &sampleGainSet[4]);
	MB_Slave_DefineReg(13, &sampleGainSet[5]);
	MB_Slave_DefineReg(14, &sampleGainSet[6]);
	MB_Slave_DefineReg(15, &sampleGainSet[7]);

	MB_Slave_DefineReg(16, &sampleRefSet[0]);
	MB_Slave_DefineReg(17, &sampleRefSet[1]);
	MB_Slave_DefineReg(18, &sampleRefSet[2]);
	MB_Slave_DefineReg(19, &sampleRefSet[3]);
	MB_Slave_DefineReg(20, &sampleRefSet[4]);
	MB_Slave_DefineReg(21, &sampleRefSet[5]);
	MB_Slave_DefineReg(22, &sampleRefSet[6]);
	MB_Slave_DefineReg(23, &sampleRefSet[7]);


	MB_Slave_DefineReg(32, &samplePara[0].gain);
	MB_Slave_DefineReg(33, &samplePara[1].gain);
	MB_Slave_DefineReg(34, &samplePara[2].gain);
	MB_Slave_DefineReg(35, &samplePara[3].gain);
	MB_Slave_DefineReg(36, &samplePara[4].gain);
	MB_Slave_DefineReg(37, &samplePara[5].gain);
	MB_Slave_DefineReg(38, &samplePara[6].gain);
	MB_Slave_DefineReg(39, &samplePara[7].gain);

	MB_Slave_DefineReg(40, &samplePara[0].ref);
	MB_Slave_DefineReg(41, &samplePara[1].ref);
	MB_Slave_DefineReg(42, &samplePara[2].ref);
	MB_Slave_DefineReg(43, &samplePara[3].ref);
	MB_Slave_DefineReg(44, &samplePara[4].ref);
	MB_Slave_DefineReg(45, &samplePara[5].ref);
	MB_Slave_DefineReg(46, &samplePara[6].ref);
	MB_Slave_DefineReg(47, &samplePara[7].ref);
}


void MB_UpdateStatus(void)
{

}

void MB_CommandParse(void)
{
	if (sampleCtrlWord == lastCtrlWord) {
		return; 		// No commands
	}

	if (lastCtrlWord == 0 && sampleCtrlWord == 1) {
		switch (sampleCtrlCmd) {
		case SAMPLE_RESET_GAIN_REF:
			SampleCtrl_Reset();
			break;

		case SAMPLE_UPDATE_GAIN:
			SampleCtrl_UpdateGains(sampleCtrlEnCh, sampleGainSet);
			break;

		case SAMPLE_UPDATE_REF:
			SampleCtrl_UpdateRefs(sampleCtrlEnCh, sampleRefSet);
			break;

		case SAMPLE_UPDATE_GAIN_REF:
			SampleCtrl_UpdateGainRef(sampleCtrlEnCh, sampleGainSet, sampleRefSet);
			break;

		default:
			break;
		}
	}

	lastCtrlWord = sampleCtrlWord;
}
