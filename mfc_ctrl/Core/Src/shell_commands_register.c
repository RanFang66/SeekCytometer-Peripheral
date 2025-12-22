/*
 * shell_commands_register.c
 *
 *  Created on: 2025年12月1日
 *      Author: ranfa
 */

#include "debug_shell.h"
#include <stdlib.h>

#include "propo_valve_drive.h"
#include "sol_valve_control.h"
#include "hsc_spi.h"
#include "hsc_conv.h"
#include "press_control.h"

static const char propoValveCmdHelp[] = "Proportional valve control, Usage: propo -s/d/a/i <id> <value>";

static void propoValveCommand(int argc, char *argv[])
{
	if (argc < 2) {
		Shell_Print(propoValveCmdHelp);
		return;
	}

	uint8_t id = 0;
	uint16_t value = 0;
	uint16_t values[PROPO_VALVE_NUM];
	if (argc >= 3) {
		id = atoi(argv[2]);
	}
	if (argc >= 4) {
		value = atoi(argv[3]);
	}

	if (argv[1][0] == '-') {
		switch (argv[1][1]) {
		case 's':
		case 'S':
			PropoValveDrive_SetAndUpdate(id, value);
			break;

		case 'd':
		case 'D':
			PropoValveDrive_Close(id);
			break;

		case 'a':
		case 'A':
			for (int i = 0; i < PROPO_VALVE_NUM; i++) {
				values[i] = value;
			}
			PropoValveDrive_SetAllAndUpdate(values);
			break;

		case 'i':
		case 'I':
			Shell_Print("\r\n>>PropoValve output value-0: %d, valve-1: %d, valve-2: %d, valve-3: %d, valve-4: %d",
					PropoValveDrive_GetValue(PROPO_VALVE_0),
					PropoValveDrive_GetValue(PROPO_VALVE_1),
					PropoValveDrive_GetValue(PROPO_VALVE_2),
					PropoValveDrive_GetValue(PROPO_VALVE_3),
					PropoValveDrive_GetValue(PROPO_VALVE_4));
			break;

		default:
			Shell_Print(propoValveCmdHelp);
			break;
		}
	} else {
		Shell_Print(propoValveCmdHelp);
	}
}

static const  DebugCommand_t propoCmd = {"propo", propoValveCmdHelp, propoValveCommand};




static const char soleValveCmdHelp[] = "SOL valve control command, Usage: sole -o/O/d/D/i/I <id> <value>";
static const char *soleValveStatusStr[] = {"Closed", "Opened"};
static void soleValveCommand(int argc, char *argv[])
{
	if (argc < 2) {
		Shell_Print(soleValveCmdHelp);
		return;
	}

	uint8_t id = 0;


	if (argc >= 3) {
		id = atoi(argv[2]);
	}


	if (argv[1][0] == '-') {
		switch (argv[1][1]) {
		case 'O':
			SOL_OpenAll();
			break;

		case 'o':
			SOL_Open(id);
			break;

		case 'd':
			SOL_Close(id);
			break;
		case 'D':
			SOL_CloseAll();
			break;


		case 'i':
		case 'I':
			Shell_Print(">>\r\nSolenoid value-0: %s; valve-1:%s, valve-2: %s; valve-3: %s; valve-4: %s",
					soleValveStatusStr[SOL_GetState(0)],
					soleValveStatusStr[SOL_GetState(1)],
					soleValveStatusStr[SOL_GetState(2)],
					soleValveStatusStr[SOL_GetState(3)],
					soleValveStatusStr[SOL_GetState(4)]);
			break;

		default:
			Shell_Print(soleValveCmdHelp);
			break;
		}
	} else {
		Shell_Print(soleValveCmdHelp);
	}
}

static const  DebugCommand_t soleCmd = {"sole", soleValveCmdHelp, soleValveCommand};


static const char pressCmdHelp[] = "Press control command, Usage: press -s/d/p/i <ch> <value>";

static const char *statusStringList[] = {
		"IDLE",
		"Running",
		"Fault",
};


static void pressCommand(int argc, char *argv[])
{
	if (argc < 3) {
		Shell_Print(pressCmdHelp);
		return;
	}

	uint8_t id = 0;
	uint16_t targetList[PRESS_CTRL_CH_NUM];
	if (argc >= 3) {
		id = atoi(argv[2]);
		if (id > PRESS_CH_ALL) {
			Shell_Print("\r\n>> Invalid channel id");
			return;
		}
	}


	if (argv[1][0] == '-') {
		switch (argv[1][1]) {
		case 's':
		case 'S':
			if (argc < 4) {
				Shell_Print("\r\n>> Use press -s ch target0, target1...");
				return;
			}
			for (uint8_t i = 0; i < PRESS_CTRL_CH_NUM; i++) {
				if (i + 3 < argc) {
					targetList[i] = atoi(argv[i+3]);
				} else {
					targetList[i] = targetList[i-1];
				}
			}

			PressCtrl_Start(id, targetList);
			break;

		case 'd':
		case 'D':
			PressCtrl_Stop(id);
			break;

		case 'p':
		case 'P':
			if (argc < 4) {
				Shell_Print("\r\n>> Use press -p ch target0, target1...");
				return;
			}
			for (uint8_t i = 0; i < PRESS_CTRL_CH_NUM; i++) {
				if (i + 3 < argc) {
					targetList[i] = atoi(argv[i+3]);
				} else {
					targetList[i] = targetList[i-1];
				}
			}
			PressCtrl_SetTarget(id, targetList);
			break;

		case 'i':
		case 'I':
			for (uint8_t i = 0; i < PRESS_CTRL_CH_NUM; i++) {
				if ((id & (0x01 << i)) != 0) {
					Shell_Print("\r\n>> Press Ch-%d: press: %d, target: %d, status: %s",
							i, (int)PressCtrl_GetLatestPress(i), PressCtrl_GetTarget(i), statusStringList[PressCtrl_GetStatus(i)]);
				}
			}
			break;

		case 'c':
		case 'C':
			Shell_Print("\r\n>> Input press: %d", (int)PressCtrl_GetInputPress());
			break;

		default:
			Shell_Print(pressCmdHelp);
			break;
		}
	} else {
		Shell_Print(pressCmdHelp);
	}
}

static const  DebugCommand_t pressCmd = {"press", pressCmdHelp, pressCommand};



void registerDebugCommands(void)
{
	bool ret;
	ret = Shell_RegisterCommand(&propoCmd);
	if (ret) {
		LOG_INFO("Register propo command OK");
	} else {
		LOG_WARNING("Register propo command FAILED");
	}

	ret = Shell_RegisterCommand(&soleCmd);
	if (ret) {
		LOG_INFO("Register sole command OK");
	} else {
		LOG_WARNING("Register sole command FAILED");
	}

	ret = Shell_RegisterCommand(&pressCmd);
	if (ret) {
		LOG_INFO("Register press command OK");
	} else {
		LOG_WARNING("Register press command FAILED");
	}
}


