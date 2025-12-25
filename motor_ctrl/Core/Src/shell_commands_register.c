/*
 * shell_commands_register.c
 *
 *  Created on: 2025年12月1日
 *      Author: ranfa
 */

#include "debug_shell.h"
#include <stdlib.h>

#include "cover_control.h"
#include "seal_control.h"
#include "bsp_adc.h"
#include "churn_control.h"
#include "temperature_control.h"
#include "stepper_motor_control.h"
#include "laser_control.h"
#include "led_control.h"

static const char coverCmdHelp[] = "Control cover(open, close), Use cover -o/c/s/i/...";
static const char *coverPosStr[] = {"Closed", "Almost Closed", "Middle", "Almost Opened", "Opened", "Undefined"};
static const char *coverStatusStr[] = {"IDLE", "OPENING", "CLOSING", "FAULT"};
static void CoverControlCommand(int argc, char *argv[])
{
	if (argc < 2) {
		Shell_Print("\r\n>> %s", coverCmdHelp);
		return;
	}

	if (argv[1][0] != '-') {
		Shell_Print("\r\n>> %s", coverCmdHelp);
		return;
	}
	uint16_t val = 0;
	switch (argv[1][1]) {
	case 'o':
	case 'O':
		Cover_Open();
		break;

	case 'c':
	case 'C':
		Cover_Close();
		break;

	case 's':
	case 'S':
		Cover_Stop();
		break;

	case 'i':
	case 'I':
		CoverStatus_t status = Cover_GetStatus();
		CoverPos_t pos = Cover_GetPos();
		Shell_Print("\r\n Cover Status: %s, Cover Pos: %s", coverStatusStr[status], coverPosStr[pos]);
		break;

	case 'v':
		if (argc < 4) {
			Shell_Print("\r\n>> Use cover -v <a/n> <val>");
			return;
		}

		val = atoi(argv[3]);
		if (argv[2][0] == 'a') {
			Cover_SetAccSpeed(val);
		} else if (argv[2][0] == 'n') {
			Cover_SetNormalSpeed(val);
		} else if (argv[2][0] == 'd') {
			Cover_SetDescSpeed(val);
		} else {
			Shell_Print("\r\n>> Use cover -v <a/n/d> <val>");
			return;
		}
		break;
	default:
		Shell_Print("\r\n>> %s", coverCmdHelp);
		break;
	}
}

static DebugCommand_t coverCmd = {"cover", coverCmdHelp, CoverControlCommand};

static const char sealCmdHelp[]  = "Control seal(push, release), Use seal -p/r/s/i/...";
static const char *sealStatusStr[] = {"IDLE", "PUSHING", "RELEASING", "FAULT"};
extern uint16_t getMaxI();
static void SealControlCommand(int argc, char *argv[])
{
	if (argc < 2) {
		Shell_Print("\r\n>> %s", sealCmdHelp);
		return;
	}

	if (argv[1][0] != '-') {
		Shell_Print("\r\n>> %s", sealCmdHelp);
		return;
	}
	uint16_t val = 0;
	switch (argv[1][1]) {
	case 'p':
	case 'P':
		SealCtrl_Push();
		break;

	case 'r':
	case 'R':
		SealCtrl_Release();
		break;

	case 's':
	case 'S':
		SealCtrl_Stop();
		break;

	case 'f':
	case 'F':
		SealCtrl_Reset();
		break;

	case 'i':
		Shell_Print("\r\n Seal Status: %s, Seal Motor Current: %d, is Released: %d, maxI: %d",
				sealStatusStr[SealCtrl_GetStatus()], GetMotorCurrentAdc(), SealCtrl_SealReleased(), getMaxI());
		break;


	case 'I':
		Shell_Print("\r\n Seal Status: %s, Seal Motor Current: %d, is Released: %d, Speed: %d(push), %d(release), time limit: (push)%d, (release)%d, fault current: %d, release current: %d",
				sealStatusStr[SealCtrl_GetStatus()], GetMotorCurrentAdc(), SealCtrl_SealReleased(),
				SealCtrl_GetPushSpeed(), SealCtrl_GetReleaseSpeed(),
				SealCtrl_GetPushTimeLimit(), SealCtrl_GetReleaseTimeLimit(),
				SealCtrl_GetFaultCurrThresh(), SealCtrl_GetPushedCurrThresh());
		break;

	case 'v':
	case 'V':
		if (argc < 4) {
			Shell_Print("\r\n>> Use seal -v <p/r> <val>");
			return;
		}

		val = atoi(argv[3]);
		if (argv[2][0] == 'p') {
			SealCtrl_SetPushSpeed(val);
		} else if (argv[2][0] == 'r') {
			SealCtrl_SetReleaseSpeed(val);
		} else {
			Shell_Print("\r\n>> Use seal -v <p/r> <val>");
			return;
		}
		break;

	case 't':
	case 'T':
		if (argc < 4) {
			Shell_Print("\r\n>> Use seal -t <p/r> <val>");
			return;
		}

		val = atoi(argv[3]);
		if (argv[2][0] == 'p') {
			SealCtrl_SetPushTimeLimit(val);
		} else if (argv[2][0] == 'r') {
			SealCtrl_SetReleaseTimeLimit(val);
		} else {
			Shell_Print("\r\n>> Use seal -t <p/r> <val>");
			return;
		}
		break;

	case 'c':
	case 'C':
		if (argc < 4) {
			Shell_Print("\r\n>> Use seal -c <f/p> <val>");
			return;
		}

		val = atoi(argv[3]);
		if (argv[2][0] == 'f') {
			sealCtrl_SetMotorFaultThresh(val);
		} else if (argv[2][0] == 'p') {
			sealCtrl_SetMotorPushedThresh(val);
		} else {
			Shell_Print("\r\n>> Use seal -c <f/p> <val>");
			return;
		}
		break;

	default:
		Shell_Print("\r\n>> %s", sealCmdHelp);
		break;
	}
}


static DebugCommand_t sealCmd = {"seal", sealCmdHelp, SealControlCommand};



const char churnCmdHelp[] = {"Churn control(run CW/CCW), Use churn -f/b/s/i <freq>"};
const char *churnStatusStr[] = {"IDLE", "Running CW", "Running CCW", "FAULT"};

static void ChurnControlCommand(int argc, char *argv[])
{
	if (argc < 2) {
		Shell_Print("\r\n>> %s", churnCmdHelp);
		return;
	}

	if (argv[1][0] != '-') {
		Shell_Print("\r\n>> %s", churnCmdHelp);
		return;
	}

	uint16_t speed;
	switch (argv[1][1]) {
		case 'f':
		case 'F':
			if (argc < 3) {
				Shell_Print("\r\n>> Use churn -f <freq>");
				return;
			}
			speed = atoi(argv[2]);
			ChurnCtrl_RunCW(speed);
			break;

		case 'b':
		case 'B':
			if (argc < 3) {
				Shell_Print("\r\n>> Use churn -b <freq>");
				return;
			}
			speed = atoi(argv[2]);
			ChurnCtrl_RunCCW(speed);
			break;

		case 's':
		case 'S':
			ChurnCtrl_Stop();
			break;

		case 'i':
		case 'I':
			Shell_Print("Churn Status: %s, Churn Speed: %d/320 r/s",
					churnStatusStr[ChurnCtrl_GetStatus()], ChurnCtrl_GetSpeed());
			break;

		default:
			Shell_Print("\r\n>> %s", churnCmdHelp);
			break;
	}
}
static DebugCommand_t churnCmd = {"churn", churnCmdHelp, ChurnControlCommand};



static const char tempCmdHelp[] = "Control temperature， Use temp -e/s/r/i/t...";
static const char *tempStatusStr[] = {"IDLE", "RUNNING", "FAULT"};

static void TempControlCommand(int argc, char *argv[])
{
	if (argc < 2) {
		Shell_Print("\r\n>> %s", tempCmdHelp);
		return;
	}

	if (argv[1][0] != '-') {
		Shell_Print("\r\n>> %s", tempCmdHelp);
		return;
	}

	uint16_t val = 0;
	float target = 0;

	switch (argv[1][1]) {
		case 'e':
		case 'E':
			if (argc < 3) {
				Shell_Print("\r\n>> Use: temp -e <target>");
				return;
			}
			val = atoi(argv[2]);
			target = (float)val / 10.0;
			TempCtrl_Start(target);
			break;

		case 's':
		case 'S':
			TempCtrl_Stop();
			break;

		case 'r':
		case 'R':
			TempCtrl_Reset();
			break;

		case 'i':
			Shell_Print("TempCtrl Status: %s, Target: %d, Measured: %d",
					tempStatusStr[TempCtrl_GetStatus()], (int)(TempCtrl_GetTempTarget()*10), (int)(TempCtrl_GetTempLatest()*10));
			break;
		case 'I':
			Shell_Print("TempCtrl Status: %s, Target: %d, Measured: %d, Kp: %d, Ki: %d",
						tempStatusStr[TempCtrl_GetStatus()], (int)(TempCtrl_GetTempTarget()*10),
						(int)(TempCtrl_GetTempLatest()*10), (int)(TempCtrl_GetKp()*100), (int)(TempCtrl_GetKi()*100));
			break;
		case 't':
		case 'T':
			if (argc < 3) {
				Shell_Print("\r\n>> Use temp -t <target>");
				return;
			}
			val = atoi(argv[2]);
			target = (float)val / 10.0;
			TempCtrl_SetTarget(target);
			break;
		default:
			Shell_Print("\r\n>> %s", tempCmdHelp);
			break;
	}
}

static DebugCommand_t tempCmd = {"temp", tempCmdHelp, TempControlCommand};


static const char fanCmdHelp[] = "Cool fan control， Use fan -e/s/i/v...";
static const char *fanStatusStr[] = {"IDLE", "RUNNING", "FAULT"};

static void FanControlCommand(int argc, char *argv[])
{
	if (argc < 2) {
		Shell_Print("\r\n>> %s", fanCmdHelp);
		return;
	}

	if (argv[1][0] != '-') {
		Shell_Print("\r\n>> %s", fanCmdHelp);
		return;
	}


	uint8_t ch = 0;
	uint16_t speed = 0;
	if (argc > 2) {
		ch = atoi(argv[2]);
		if (ch > 0x0F) {
			Shell_Print("\r\n>> Invalid cool fan id select");
			return;
		}
	}
	if (argc > 3) {
		speed = atoi(argv[3]);
	}



	switch (argv[1][1]) {
	case 'e':
	case 'E':
		TempCtrl_EnableFan(ch);
		break;

	case 's':
	case 'S':
		TempCtrl_DisableFan(ch);
		break;

	case 'v':
	case 'V':
		if (argc < 4) {
			Shell_Print("\r\n>> %s Use fan -v <ch> speed");
			break;
		}
		speed = atoi(argv[3]);
		TempCtrl_SetFanSpeed(ch, speed);
		break;

	case 'i':
	case 'I':
		Shell_Print("\r\n>> Fan-1: %s, %d, Fan-2: %s, %d, Fan-3: %s, %d, Fan-4: %s, %d",
				fanStatusStr[TempCtrl_GetFanStatus(0)], TempCtrl_GetFanSpeed(0),
				fanStatusStr[TempCtrl_GetFanStatus(1)], TempCtrl_GetFanSpeed(1),
				fanStatusStr[TempCtrl_GetFanStatus(2)], TempCtrl_GetFanSpeed(2),
				fanStatusStr[TempCtrl_GetFanStatus(3)], TempCtrl_GetFanSpeed(3));
		break;
	default:
		Shell_Print("\r\n>> %s", fanCmdHelp);
		return;
	}
}
static DebugCommand_t fanCmd = {"fan", fanCmdHelp, FanControlCommand};



static const char SMotorCmdHelp[] = "Control Stepper Motor， Use motor -s/p/m/o/r/i <id> <val>...";

static void SMotorControlCommand(int argc, char *argv[])
{
	if (argc < 3) {
		Shell_Print("\r\n>> %s", SMotorCmdHelp);
		return;
	}

	if (argv[1][0] != '-') {
		Shell_Print("\r\n>> %s", SMotorCmdHelp);
		return;
	}

	SMotorIndex_t id = MOTOR_X;
	if (argv[2][0] == '0' || argv[2][0] == 'x' || argv[2][0] == 'X') {
		id = MOTOR_X;
	} else if (argv[2][0] == '1' || argv[2][0] == 'y' || argv[2][0] == 'Y') {
		id = MOTOR_Y;
	} else if (argv[2][0] == '2' || argv[2][0] == 'z' || argv[2][0] == 'Z') {
		id = MOTOR_Z;
	} else {
		Shell_Print("\r\n>> Invalid motor id, 0: x, 1: y, 2: z");
		return;
	}


	int32_t val = 0;
	switch (argv[1][1]) {
		case 's':
		case 'S':
			SMotorCtrl_Stop(id);
			break;

		case 'o':
		case 'O':
			SMotorCtrl_FindHome(id);
			break;

		case 'r':
		case 'R':
			SMotorCtrl_Reset(id);
			break;

		case 'm':
		case 'M':
			if (argc < 4) {
				Shell_Print("\r\n>> Use motor -m <id> <steps>");
				return;
			}
			val = atoi(argv[3]);
			SMotorCtrl_RunSteps(id, val);
			break;

		case 'p':
		case 'P':
			if (argc < 4) {
				Shell_Print("\r\n>> Use motor -p <id> <pos>");
				return;
			}
			val = atoi(argv[3]);
			SMotorCtrl_RunToPos(id, val);
			break;

		case 'i':
		case 'I':
			Shell_Print("Motor-%c: Status: %d, Pos: %d, Limit Status: %d",
						SMotorCtrl_GetName(id), SMotorCtrl_GetStatus(id), SMotorCtrl_GetPos(id), SMotorCtrl_GetLimitStatus(id));
			break;


		default:
			Shell_Print("\r\n>> %s", SMotorCmdHelp);
			break;
	}
}

static DebugCommand_t SMotorCtrlCmd = {"motor", SMotorCmdHelp, SMotorControlCommand};



const char laserCmdHelp[] = "Control laser on/off, Use laser -o/d/i <id> <val>";
const char *laserStatusStr[] = {"Off", "On"};

void LaserControlCommand(int argc, char *argv[])
{
	if (argc < 3) {
		Shell_Print("\r\n>> %s", laserCmdHelp);
		return;
	}

	if (argv[1][0] != '-') {
		Shell_Print("\r\n>> %s", laserCmdHelp);
		return;
	}

	LaserIndex_t id = LASER_1;
	if (argv[2][0] == '1') {
		id = LASER_1;
	} else if (argv[2][0] == '2') {
		id = LASER_2;
	} else if (argv[2][0] == 'a' || argv[2][0] == 'A') {
		id =  LASER_ALL;
	} else {
		Shell_Print("\r\n>> Invalid Laser id, id: 1, 2, a/A ");
		return;
	}

	uint16_t val = 0;
	switch (argv[1][1]) {
	case 'd':
	case 'D':
		Laser_SwitchOff(id);
		break;

	case 'o':
	case 'O':
		if (argc < 4) {
			Shell_Print("\r\n>> Use laser -o <id> <val>");
			return;
		}
		val = atoi(argv[3]);
		Laser_SwitchOn(id, val);
		break;

	case 's':
	case 'S':
		if (argc < 4) {
			Shell_Print("\r\n>> Use laser -s <id> <val>");
			return;
		}
		val = atoi(argv[3]);
		Laser_SetIntensity(id, val);
		break;



	case 'i':
	case 'I':
		if (id == LASER_ALL) {
			Shell_Print("\r\n>> Laser-1: %s, intensity: %d; Laser-2: %s, intensity: %d",
					laserStatusStr[Laser_GetStatus(LASER_1)], Laser_GetIntensity(LASER_1),
					laserStatusStr[Laser_GetStatus(LASER_2)], Laser_GetIntensity(LASER_2));
		} else {
			Shell_Print("\r\n>> Laser-%d: %s, intensity: %d",
					(uint8_t)id +1, laserStatusStr[Laser_GetStatus(id)], Laser_GetIntensity(id));
		}
		break;
	default:
		Shell_Print("\r\n>> %s", laserCmdHelp);
		break;
	}
}

static DebugCommand_t LaserCtrlCmd = {"laser", laserCmdHelp, LaserControlCommand};



const char ledCmdHelp[] = "Control LED on/off, Use led -o/d/i <val>";
const char *ledStatusStr[] = {"Off", "On"};

void LEDControlCommand(int argc, char *argv[])
{
	if (argc < 2) {
		Shell_Print("\r\n>> %s", ledCmdHelp);
		return;
	}

	if (argv[1][0] != '-') {
		Shell_Print("\r\n>> %s", ledCmdHelp);
		return;
	}

	uint16_t val = 0;
	switch (argv[1][1]) {
	case 'd':
	case 'D':
		LED_SwitchOff();
		break;

	case 'o':
	case 'O':
		if (argc < 3) {
			Shell_Print("\r\n>> Use led -o <val>");
			return;
		}
		val = atoi(argv[2]);
		LED_SwitchOn(val);
		break;

	case 's':
	case 'S':
		if (argc < 3) {
			Shell_Print("\r\n>> Use led -s <val>");
			return;
		}
		val = atoi(argv[2]);
		LED_SetIntensity(val);
		break;

	case 'i':
	case 'I':
		Shell_Print("\r\n>> LED Status: %s, intensity: %d",
				ledStatusStr[LED_GetStatus()], LED_GetIntensity());
		break;

	default:
		Shell_Print("\r\n>> %s", ledCmdHelp);
		break;
	}
}

static DebugCommand_t LEDCtrlCmd = {"led", ledCmdHelp, LEDControlCommand};




void registerDebugCommands(void)
{
	bool ret;
	ret = Shell_RegisterCommand(&coverCmd);
	if (ret) {
		LOG_INFO("Register cover command OK");
	} else {
		LOG_WARNING("Register cover command FAILED");
	}

	ret = Shell_RegisterCommand(&sealCmd);
	if (ret) {
		LOG_INFO("Register seal command OK");
	} else {
		LOG_WARNING("Register seal command FAILED");
	}

	ret = Shell_RegisterCommand(&churnCmd);
	if (ret) {
		LOG_INFO("Register churn command OK");
	} else {
		LOG_WARNING("Register churn command FAILED");
	}

	ret = Shell_RegisterCommand(&tempCmd);
	if (ret) {
		LOG_INFO("Register temp command OK");
	} else {
		LOG_WARNING("Register temp command FAILED");
	}

	ret = Shell_RegisterCommand(&fanCmd);
	if (ret) {
		LOG_INFO("Register fan command OK");
	} else {
		LOG_WARNING("Register fan command FAILED");
	}


	ret = Shell_RegisterCommand(&SMotorCtrlCmd);
	if (ret) {
		LOG_INFO("Register stepper motor command OK");
	} else {
		LOG_WARNING("Register stepper motor command FAILED");
	}

	ret = Shell_RegisterCommand(&LaserCtrlCmd);
	if (ret) {
		LOG_INFO("Register laser command OK");
	} else {
		LOG_WARNING("Register laser command FAILED");
	}

	ret = Shell_RegisterCommand(&LEDCtrlCmd);
	if (ret) {
		LOG_INFO("Register led command OK");
	} else {
		LOG_WARNING("Register led command FAILED");
	}
}


