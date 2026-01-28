/*
 * modbus_local_table.c
 *
 *  Created on: 2025年11月27日
 *      Author: ranfa
 */

#include "modbus_slave.h"
#include "cover_control.h"
#include "seal_control.h"
#include "churn_control.h"
#include "temperature_control.h"
#include "stepper_motor_control.h"
#include "laser_control.h"
#include "led_control.h"


// sample parameter defined in sample_control.c

typedef union {
	uint16_t word;
	struct BITS{
		uint16_t coverCtrl 	: 	1;
		uint16_t sealCtrl	: 	1;
		uint16_t churnCtrl 	:	1;
		uint16_t tempCtrl 	:	1;
		uint16_t SMotorCtrl	: 	1;
		uint16_t laserCtrl	:	1;
		uint16_t LEDCtrl	: 	1;
		uint16_t reserved	:   9;
	} bits;
} CtrlWord_t;


static CtrlWord_t ctrlWord = {.word = 0};
static CtrlWord_t lastCtrlWord = {.word = 0};
uint16_t coverCmd = 0;
uint16_t sealCmd = 0;
uint16_t churnCmd = 0;
uint16_t tempCmd = 0;
uint16_t motorCmd = 0;
uint16_t motorTargetSetHi = 0;
uint16_t motorTargetSetLo = 0;
uint16_t tempTargetSet = 0;
uint16_t laserCmd = 0;
uint16_t ledCmd = 0;
uint16_t churnSpeedSet = 0;
uint16_t laserIntensitySet = 0;
uint16_t ledIntensitySet = 0;
uint16_t fanChSet = 0;
uint16_t fanSpeedSet = 0;
uint16_t fanChSpeedSet[4] = {0};


uint16_t coverStatus;
uint16_t sealStatus;
uint16_t churnStatus;
uint16_t tempCtrlStatus;
uint16_t motorXStatus;
uint16_t motorYStatus;
uint16_t motorZStatus;
uint16_t motorLimitStatus;
uint16_t motorXPosHi;
uint16_t motorXPosLo;
uint16_t motorYPosHi;
uint16_t motorYPosLo;
uint16_t motorZPosHi;
uint16_t motorZPosLo;
uint16_t laser1Status;
uint16_t laser1Intensity;
uint16_t laser2Status;
uint16_t laser2Intensity;
uint16_t ledStatus;
uint16_t ledIntensity;
uint16_t currentTemp;

void MB_Local_RegInit(void)
{
	MB_Slave_DefineReg(0, &ctrlWord.word);
	MB_Slave_DefineReg(8, &coverCmd);
	MB_Slave_DefineReg(9, &sealCmd);



	MB_Slave_DefineReg(13, &churnCmd);
	MB_Slave_DefineReg(14, &churnSpeedSet);

	MB_Slave_DefineReg(15, &motorCmd);
	MB_Slave_DefineReg(16, &motorTargetSetHi);
	MB_Slave_DefineReg(17, &motorTargetSetLo);

	MB_Slave_DefineReg(18, &laserCmd);
	MB_Slave_DefineReg(19, &laserIntensitySet);

	MB_Slave_DefineReg(20, &ledCmd);
	MB_Slave_DefineReg(21, &ledIntensitySet);



	MB_Slave_DefineReg(22, &tempCmd);
	MB_Slave_DefineReg(23, &tempTargetSet);
	MB_Slave_DefineReg(24, &fanChSet);
	MB_Slave_DefineReg(25, &fanSpeedSet);
	MB_Slave_DefineReg(26, &fanChSpeedSet[0]);
	MB_Slave_DefineReg(27, &fanChSpeedSet[1]);
	MB_Slave_DefineReg(28, &fanChSpeedSet[2]);
	MB_Slave_DefineReg(29, &fanChSpeedSet[3]);


	MB_Slave_DefineReg(47, &tempCtrlStatus);
	MB_Slave_DefineReg(48, &currentTemp);

	MB_Slave_DefineReg(49, &coverStatus);
	MB_Slave_DefineReg(50, &sealStatus);
	MB_Slave_DefineReg(51, &churnStatus);
	MB_Slave_DefineReg(52, &motorXStatus);
	MB_Slave_DefineReg(53, &motorYStatus);
	MB_Slave_DefineReg(54, &motorZStatus);
	MB_Slave_DefineReg(55, &motorLimitStatus);
	MB_Slave_DefineReg(56, &motorXPosHi);
	MB_Slave_DefineReg(57, &motorXPosLo);
	MB_Slave_DefineReg(58, &motorYPosHi);
	MB_Slave_DefineReg(59, &motorYPosLo);
	MB_Slave_DefineReg(60, &motorZPosHi);
	MB_Slave_DefineReg(61, &motorZPosLo);
	MB_Slave_DefineReg(62, &laser1Status);
	MB_Slave_DefineReg(63, &laser1Intensity);
	MB_Slave_DefineReg(64, &laser2Status);
	MB_Slave_DefineReg(65, &laser2Intensity);
	MB_Slave_DefineReg(66, &ledStatus);
	MB_Slave_DefineReg(67, &ledIntensity);

}

void MB_UpdateStatus(void)
{
	coverStatus = Cover_GetStatus();
	sealStatus = SealCtrl_GetStatus();
	churnStatus = ChurnCtrl_GetStatus();
	tempCtrlStatus = TempCtrl_GetStatus();
	motorXStatus = SMotorCtrl_GetStatus(MOTOR_X);
	motorYStatus = SMotorCtrl_GetStatus(MOTOR_Y);
	motorZStatus = SMotorCtrl_GetStatus(MOTOR_Z);
	float temp = TempCtrl_GetTempLatest();
	currentTemp = temp * 10;

	uint16_t limitX = SMotorCtrl_GetLimitStatus(MOTOR_X);
	uint16_t limitY = SMotorCtrl_GetLimitStatus(MOTOR_Y);
	uint16_t limitZ = SMotorCtrl_GetLimitStatus(MOTOR_Z);
	motorLimitStatus = ((limitZ & 0x0003) << 4) | ((limitY & 0x0003) << 2) | (limitX & 0x0003);
	int32_t posX = SMotorCtrl_GetPos(MOTOR_X);
	int32_t posY = SMotorCtrl_GetPos(MOTOR_Y);
	int32_t posZ = SMotorCtrl_GetPos(MOTOR_Z);



	motorXPosHi = (uint16_t)((uint32_t)posX >> 16);
	motorXPosLo = (uint16_t)(posX &0x0000FFFF);
	motorYPosHi = (uint16_t)((uint32_t)posY >> 16);
	motorYPosLo = (uint16_t)(posY &0x0000FFFF);
	motorZPosHi = (uint16_t)((uint32_t)posZ >> 16);
	motorZPosLo = (uint16_t)(posZ &0x0000FFFF);

//	motorXPos = (uint16_t)(SMotorCtrl_GetPos(MOTOR_X) / 100);
//	motorXPos = (uint16_t)(SMotorCtrl_GetPos(MOTOR_Y) / 100);
//	motorXPos = (uint16_t)(SMotorCtrl_GetPos(MOTOR_Z) / 100);
	laser1Status = Laser_GetStatus(LASER_1);
	laser2Status = Laser_GetStatus(LASER_2);
	ledStatus = LED_GetStatus();
	laser1Intensity = Laser_GetIntensity(LASER_1);
	laser2Intensity = Laser_GetIntensity(LASER_2);
	ledIntensity = LED_GetIntensity();

}

void MB_CommandParse(void)
{
	if (ctrlWord.word == lastCtrlWord.word) {
		return; 		// No commands
	}
	if (ctrlWord.bits.coverCtrl && !lastCtrlWord.bits.coverCtrl) {
		switch (coverCmd) {
		case COVER_CMD_STOP:
			Cover_Stop();
			break;
		case COVER_CMD_OPEN:
			Cover_Open();
			break;
		case COVER_CMD_CLOSE:
			Cover_Close();
			break;
		default:
			break;
		}
	}

	if (ctrlWord.bits.sealCtrl && !lastCtrlWord.bits.sealCtrl) {
		switch (sealCmd) {
		case SEAL_CMD_STOP:
			SealCtrl_Stop();
			break;

		case SEAL_CMD_PUSH:
			SealCtrl_Push();
			break;

		case SEAL_CMD_RELEASE:
			SealCtrl_Release();
			break;

		case SEAL_CMD_RESET:
			SealCtrl_Reset();
			break;

		default:
			break;
		}
	}

	if (ctrlWord.bits.churnCtrl && !lastCtrlWord.bits.churnCtrl) {
		switch (churnCmd) {
		case CHURN_CMD_STOP:
			ChurnCtrl_Stop();
			break;

		case CHURN_CMD_RUN_CW:
			ChurnCtrl_RunCW(churnSpeedSet);
			break;

		case CHURN_CMD_RUN_CCW:
			ChurnCtrl_RunCCW(churnSpeedSet);
			break;

		case CHURN_CMD_RESET:
			ChurnCtrl_Reset();
			break;

		default:
			break;
		}
	}

	if (ctrlWord.bits.tempCtrl && !lastCtrlWord.bits.tempCtrl) {
		switch (tempCmd) {
		case TEMP_CTRL_CMD_STOP:
			TempCtrl_Stop();
			break;
		case TEMP_CTRL_CMD_START:
			TempCtrl_Start(tempTargetSet);
			break;

		case TEMP_CTRL_SET_TARGET:
			TempCtrl_SetTarget(tempTargetSet);
			break;

		case TEMP_CTRL_FAN_SET:
			TempCtrl_FanSet(fanChSet, fanChSpeedSet);
			break;

		case TEMP_CTRL_FAN_ENABLE:
			TempCtrl_EnableFan(fanChSet);
			break;

		case TEMP_CTRL_FAN_DISABLE:
			TempCtrl_DisableFan(fanChSet);
			break;

		case TEMP_CTRL_FAN_SET_SPEED:
			TempCtrl_SetFanSpeed(fanChSet, fanSpeedSet);
			break;


		case TEMP_CTRL_CMD_RESET:
			TempCtrl_Reset();
			break;
		default:
			break;
		}
	}

	if (ctrlWord.bits.SMotorCtrl && !lastCtrlWord.bits.SMotorCtrl) {
		uint8_t cmdType = motorCmd & 0x00FF;
		uint8_t id = (motorCmd >> 8) & 0x00FF;
		int32_t val = 0;
		switch (cmdType) {
		case STEPPER_MOTOR_STOP:
			SMotorCtrl_Stop(id);
			break;

		case STEPPER_MOTOR_RUN_STEPS:
			val = (int32_t)((motorTargetSetHi << 16) | (motorTargetSetLo));
			SMotorCtrl_RunSteps(id, val);
			break;

		case STEPPER_MOTOR_RUN_POS:
			val = (int32_t)((motorTargetSetHi << 16) | (motorTargetSetLo));
			SMotorCtrl_RunToPos(id, val);
			break;

		case STEPPER_MOTOR_FIND_HOME:
			SMotorCtrl_FindHome(id);
			break;

		case STEPPER_MOTOR_RESET:
			SMotorCtrl_Reset(id);
			break;

		default:
			break;
		}
	}

	if (ctrlWord.bits.laserCtrl && !lastCtrlWord.bits.laserCtrl) {
		uint8_t laserId = (laserCmd >> 8) & 0x00FF;
		uint8_t cmdType = laserCmd & 0x00FF;

		switch (cmdType) {
		case LASER_CMD_SWITCH_OFF:
			Laser_SwitchOff(laserId);
			break;

		case LASER_CMD_SWITCH_ON:
			Laser_SwitchOn(laserId, laserIntensitySet);
			break;

		case LASER_CMD_SET_INTENSITY:
			Laser_SetIntensity(laserId, laserIntensitySet);
			break;
		default:
			break;
		}
	}


	if (ctrlWord.bits.LEDCtrl && !lastCtrlWord.bits.LEDCtrl) {
		switch (ledCmd) {
		case LED_CMD_SWITCH_OFF:
			LED_SwitchOff();
			break;

		case LED_CMD_SWITCH_ON:
			LED_SwitchOn(ledIntensitySet);
			break;

		case LED_CMD_SET_INTENSITY:
			LED_SetIntensity(ledIntensitySet);
			break;
		default:
			break;
		}
	}

	lastCtrlWord.word = ctrlWord.word;
}

