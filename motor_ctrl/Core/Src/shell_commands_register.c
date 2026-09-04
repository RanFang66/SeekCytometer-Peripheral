/*
 * shell_commands_register.c
 *
 *  Created on: 2025年12月1日
 *      Author: ranfa
 */

#include "debug_shell.h"
#include "modbus_master.h"
#include "modbus_slave.h"
#include <stdlib.h>

#include "cover_control.h"
#include "seal_control.h"
#include "bsp_adc.h"
#include "churn_control.h"
#include "churn_control_dc.h"
#include "temperature_control.h"
#include "stepper_motor_control.h"
#include "laser_control.h"
#include "led_control.h"

#if ENABLE_COVER_CTRL
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
#endif /* ENABLE_COVER_CTRL */

static const char sealCmdHelp[]  = "Control seal(push, release), Use seal -p/r/s/i/... , raw H-bridge: seal -d <in1duty 0~10000> <in2 0/1>";
static const char *sealStatusStr[] = {"IDLE", "PUSHING", "PUSHED", "RELEASING", "RELEASED","FAULT"};
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
		Shell_Print("\r\n Seal Status: %s, Seal Motor Current: %d, Sensor IO: %d, maxI: %d",
				sealStatusStr[SealCtrl_GetStatus()], GetMotorCurrentAdc(), SealCtrl_SealPushed(), getMaxI());
		Shell_Print("\r\n H-bridge: IN1(PA10) CCR=%d level=%d, IN2(PA8) level=%d, PWR(PC7)=%d",
				SealCtrl_GetIn1Compare(), SealCtrl_GetIn1Level(),
				SealCtrl_GetIn2Level(), SealCtrl_GetPowerLevel());
		break;

	/* Raw H-bridge drive for bench bring-up: seal -d <in1 duty 0~10000> <in2 0/1> */
	case 'd':
	case 'D':
		if (argc < 4) {
			Shell_Print("\r\n>> Use seal -d <in1duty 0~10000> <in2 0/1>");
			return;
		}
		val = atoi(argv[2]);
		SealCtrl_RawDrive(val, (uint8_t)atoi(argv[3]));
		Shell_Print("\r\n Raw drive: IN1 duty=%d -> CCR=%d, IN2=%d (%s)",
				val, SealCtrl_GetIn1Compare(), SealCtrl_GetIn2Level(),
				(atoi(argv[3]) != 0) ? "reverse/slow decay" : "forward/fast decay");
		break;


	case 'I':
		Shell_Print("\r\n Seal Status: %s, Seal Motor Current: %d, Sensor IO: %d, Speed: %d(push), %d(release), time limit: (push)%d, (release)%d, fault current: %d, release current: %d",
				sealStatusStr[SealCtrl_GetStatus()], GetMotorCurrentAdc(), SealCtrl_SealPushed(),
				SealCtrl_GetPushSpeed(), SealCtrl_GetReleaseSpeed(),
				SealCtrl_GetPushTimeLimit(), SealCtrl_GetReleaseTimeLimit(),
				SealCtrl_GetFaultCurrThresh(), SealCtrl_GetPushedCurrThresh());
		Shell_Print("\r\n Extra push time after sensor: %d ms", SealCtrl_GetPushExtraTime());
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
			Shell_Print("\r\n>> Use seal -t <p/r/e> <val>");
			return;
		}

		val = atoi(argv[3]);
		if (argv[2][0] == 'p') {
			SealCtrl_SetPushTimeLimit(val);
		} else if (argv[2][0] == 'r') {
			SealCtrl_SetReleaseTimeLimit(val);
		} else if (argv[2][0] == 'e') {
			SealCtrl_SetPushExtraTime(val);
		} else {
			Shell_Print("\r\n>> Use seal -t <p/r/e> <val>");
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


const char churnDcCmdHelp[] = {"DC churn control, Use churndc -r <duty 0~100> / -s(stop) / -i(info)"};
const char *churnDcStatusStr[] = {"IDLE", "RUNNING", "FAULT"};

static void ChurnDcControlCommand(int argc, char *argv[])
{
	if (argc < 2 || argv[1][0] != '-') {
		Shell_Print("\r\n>> %s", churnDcCmdHelp);
		return;
	}

	uint8_t duty;
	switch (argv[1][1]) {
		case 'r':
		case 'R':
			if (argc < 3) {
				Shell_Print("\r\n>> Use churndc -r <duty 0~100>");
				return;
			}
			duty = (uint8_t)atoi(argv[2]);
			if (duty > 100) {
				duty = 100;
			}
			ChurnDcCtrl_Start(duty);
			break;

		case 's':
		case 'S':
			ChurnDcCtrl_Stop();
			break;

		case 'i':
		case 'I':
			Shell_Print("Churn DC Status: %s, Duty: %d%%",
					churnDcStatusStr[ChurnDcCtrl_GetStatus()], ChurnDcCtrl_GetSpeed());
			break;

		default:
			Shell_Print("\r\n>> %s", churnDcCmdHelp);
			break;
	}
}
static DebugCommand_t churnDcCmd = {"churndc", churnDcCmdHelp, ChurnDcControlCommand};



static const char tempCmdHelp[] = "Control temperature, Use temp -e/s/r/i/I/t/k/f/p/w ...";
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
				Shell_Print("\r\n>> Use: temp -e <target*10>");
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
			Shell_Print("TempCtrl Status: %s, Target: %d, Air: %d, Sink1: %d, Sink2: %d, Output: %d",
					tempStatusStr[TempCtrl_GetStatus()], (int)(TempCtrl_GetTempTarget()*10), (int)(TempCtrl_GetTemp(TEMPERATURE_AIR_INDEX)*10),
					(int)(TempCtrl_GetTemp(TEMPERATURE_SINK_1_INDEX)*10), (int)(TempCtrl_GetTemp(TEMPERATURE_SINK_2_INDEX)*10),(int)(TempCtrl_GetTemp(TEMPERATURE_OUTPUT_INDEX)*10));
			break;
		case 'I': {
			/* Temperatures are printed x10; gains and PID terms x10 as well, so the
			 * same scaling used by "temp -k"/"temp -f" reads back directly. Split
			 * over two lines - SHELL_PRINT_BUFFER_SIZE is only 256 bytes. */
			float dP = 0, dI = 0, dD = 0, ff = 0;
			TempCtrl_GetPidDebug(&dP, &dI, &dD, &ff);
			Shell_Print("TempCtrl Status: %s, Target: %d, Air: %d, Sink1: %d, Sink2: %d, Out: %d",
						tempStatusStr[TempCtrl_GetStatus()], (int)(TempCtrl_GetTempTarget()*10),
						(int)(TempCtrl_GetTemp(TEMPERATURE_AIR_INDEX)*10),
						(int)(TempCtrl_GetTemp(TEMPERATURE_SINK_1_INDEX)*10),
						(int)(TempCtrl_GetTemp(TEMPERATURE_SINK_2_INDEX)*10),
						(int)TempCtrl_GetOutput());
			Shell_Print("\r\n   PID Kp: %d, Ki: %d, Kd: %d, FFcoee: %d | dP: %d, dI: %d, dD: %d, ff: %d",
						(int)(TempCtrl_GetKp()*10), (int)(TempCtrl_GetKi()*10),
						(int)(TempCtrl_GetKd()*10), (int)(TempCtrl_GetFfCoee()*10),
						(int)dP, (int)dI, (int)dD, (int)ff);
			Shell_Print("\r\n   PWM %lu Hz, peltier out: %d / %d (interleaved: %s)",
						(unsigned long)TempCtrl_GetPwmFreq(),
						TempCtrl_GetPeltierOutput(0), TempCtrl_GetPeltierOutput(1),
						TEMP_PELTIER_INTERLEAVE ? "yes" : "no");
			break;
		}
		case 't':
		case 'T':
			if (argc < 3) {
				Shell_Print("\r\n>> Use temp -t <target*10>");
				return;
			}
			val = atoi(argv[2]);
			target = (float)val / 10.0;
			TempCtrl_SetTarget(target);
			break;

		case 'k':
		case 'K': {
			/* Kp reaches the thousands, so this cannot reuse the uint16_t val */
			if (argc < 5) {
				Shell_Print("\r\n>> Use temp -k <kp*10> <ki*10> <kd*10>");
				return;
			}
			int32_t kp = atoi(argv[2]);
			int32_t ki = atoi(argv[3]);
			int32_t kd = atoi(argv[4]);
			TempCtrl_SetTunings((float)kp / 10.0f, (float)ki / 10.0f, (float)kd / 10.0f);
			Shell_Print("Set PID tunings (x10): Kp: %d, Ki: %d, Kd: %d", (int)kp, (int)ki, (int)kd);
			break;
		}

		case 'f':
		case 'F': {
			if (argc < 3) {
				Shell_Print("\r\n>> Use temp -f <ffCoee*10>");
				return;
			}
			int32_t ffCoee = atoi(argv[2]);
			TempCtrl_SetFeedforwardCoee((float)ffCoee / 10.0f);
			Shell_Print("Set feedforward coefficient (x10): %d", (int)ffCoee);
			break;
		}

		case 'p':
		case 'P': {
			/* Open-loop bench drive, for looking at the switching noise on a
			 * scope. 0 and 100 are the two static cases (no edges at all). */
			if (argc < 3) {
				Shell_Print("\r\n>> Use temp -p <duty 0~100 %%>");
				return;
			}
			int32_t duty = atoi(argv[2]);
			if (duty < 0) duty = 0;
			if (duty > 100) duty = 100;
			TempCtrl_RawPwm((uint16_t)duty);
			Shell_Print("Closed loop OFF, peltier PWM forced to %d %% at %lu Hz",
						(int)duty, (unsigned long)TempCtrl_GetPwmFreq());
			break;
		}

		case 'w':
		case 'W': {
			if (argc < 3) {
				Shell_Print("\r\n>> Use temp -w <freq Hz>");
				return;
			}
			int32_t hz = atoi(argv[2]);
			if (hz < 0) hz = 0;
			uint32_t actual = TempCtrl_SetPwmFreq((uint32_t)hz);
			Shell_Print("Peltier PWM frequency: %lu Hz (requested %d)",
						(unsigned long)actual, (int)hz);
			break;
		}

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
			if (argc < 4) {
				Shell_Print("\r\n>> Use motor -m <id> <steps>");
				return;
			}
			val = atoi(argv[3]);
			SMotorCtrl_RunSteps(id, val, 0);
			break;

		case 'M':
			if (argc < 4) {
				Shell_Print("\r\n>> Use motor -m <id> <steps>");
				return;
			}
			val = atoi(argv[3]);
			SMotorCtrl_RunSteps(id, val, 1);
			break;


		case 'p':
			if (argc < 4) {
				Shell_Print("\r\n>> Use motor -p <id> <pos>");
				return;
			}
			val = atoi(argv[3]);
			SMotorCtrl_RunToPos(id, val, 0);
			break;

		case 'P':
			if (argc < 4) {
				Shell_Print("\r\n>> Use motor -p <id> <pos>");
				return;
			}
			val = atoi(argv[3]);
			SMotorCtrl_RunToPos(id, val, 1);
			break;


		case 'i':
		case 'I':
			Shell_Print("Motor-%c: Status: 0x%04X, Pos: %d, Limit Status: %d, PP ready: %d",
						SMotorCtrl_GetName(id), SMotorCtrl_GetStatus(id), SMotorCtrl_GetPos(id),
						SMotorCtrl_GetLimitStatus(id), SMotorCtrl_IsReady(id));
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





/* --- Modbus link diagnostics ---------------------------------------------
 * "mb -i" tells a dead link apart from a noisy one: errCode carries the OR of
 * every HAL_UART_ERROR_* seen, so FE/NE point at line noise while ORE points at
 * the DMA/interrupt path being held off. RxState 0x22 = BUSY_RX = armed.
 * ------------------------------------------------------------------------*/
static const char mbCmdHelp[] = "Modbus link diagnostics, Use mb -i/c";

static void MbDiagCommand(int argc, char *argv[])
{
	if (argc < 2 || argv[1][0] != '-') {
		Shell_Print("\r\n>> %s", mbCmdHelp);
		return;
	}

	MB_SlaveDiag_t d;

	switch (argv[1][1]) {
		case 'i':
		case 'I':
			MB_Slave_GetDiag(&d);
			Shell_Print("MB slave: rxEvt=%lu processed=%lu valid=%lu RxState=0x%02X%s",
						(unsigned long)d.rxEvtCount, (unsigned long)d.processCount,
						(unsigned long)d.validCount, d.rxState,
						(d.rxState == (uint8_t)HAL_UART_STATE_BUSY_RX) ? " (armed)" : " (NOT ARMED)");
			Shell_Print("\r\n   err=%lu code=0x%02lX [%s%s%s%s] rearmFail=%lu wdRearm=%lu",
						(unsigned long)d.errCount, (unsigned long)d.errCode,
						(d.errCode & HAL_UART_ERROR_ORE) ? "ORE " : "",
						(d.errCode & HAL_UART_ERROR_FE)  ? "FE "  : "",
						(d.errCode & HAL_UART_ERROR_NE)  ? "NE "  : "",
						(d.errCode & HAL_UART_ERROR_PE)  ? "PE "  : "",
						(unsigned long)d.rearmFail, (unsigned long)d.wdRearm);
			{
				uint32_t mErr = 0, mCode = 0;
				MB_Master_GetDiag(&mErr, &mCode);
				Shell_Print("\r\n   master: err=%lu code=0x%02lX",
							(unsigned long)mErr, (unsigned long)mCode);
			}
			break;

		case 'c':
		case 'C':
			MB_Slave_ClearDiag();
			Shell_Print("MB diagnostics cleared");
			break;

		default:
			Shell_Print("\r\n>> %s", mbCmdHelp);
			break;
	}
}

static const DebugCommand_t mbCmd = {"mb", mbCmdHelp, MbDiagCommand};

void registerDebugCommands(void)
{
	bool ret;
#if ENABLE_COVER_CTRL
	ret = Shell_RegisterCommand(&coverCmd);
	if (ret) {
		LOG_INFO("Register cover command OK");
	} else {
		LOG_WARNING("Register cover command FAILED");
	}
#endif

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

	ret = Shell_RegisterCommand(&churnDcCmd);
	if (ret) {
		LOG_INFO("Register churn DC command OK");
	} else {
		LOG_WARNING("Register churn DC command FAILED");
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

	ret = Shell_RegisterCommand(&mbCmd);
	if (ret) {
		LOG_INFO("Register mb command OK");
	} else {
		LOG_WARNING("Register mb command FAILED");
	}
}


