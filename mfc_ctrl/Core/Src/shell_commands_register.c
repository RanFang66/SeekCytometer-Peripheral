/*
 * shell_commands_register.c
 *
 *  Created on: 2025年12月1日
 *      Author: ranfa
 */

#include "debug_shell.h"
#include "modbus_slave.h"
#include <stdlib.h>

#include "propo_valve_drive.h"
#include "sol_valve_control.h"
#include "hsc_spi.h"
#include "hsc_conv.h"
#include "press_control.h"
#include "param_store.h"

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


static const char paramCmdHelp[] = "Param store command, Usage: param -i/s/d/e";

static void paramCommand(int argc, char *argv[])
{
	if (argc < 2 || argv[1][0] != '-') {
		Shell_Print(paramCmdHelp);
		return;
	}

	switch (argv[1][1]) {
	case 'i':
	case 'I': {
		uint16_t status = ParamStore_GetStatus();
		const ParamRecord_t *rec = ParamStore_GetCached();

		Shell_Print("\r\n>> Param store: %s%s%s%s%s",
				(status & PARAM_ST_LOADED)     ? "LOADED "    : "",
				(status & PARAM_ST_DEFAULTS)   ? "DEFAULTS "  : "",
				(status & PARAM_ST_DIRTY)      ? "DIRTY "     : "",
				(status & PARAM_ST_SAVE_ERROR) ? "SAVE_ERR "  : "",
				(status & PARAM_ST_COMPACTED)  ? "COMPACTED " : "");
		Shell_Print("\r\n>> seq: %lu, slots used: %u/%u (%u%%)",
				(unsigned long)ParamStore_GetSeq(), ParamStore_GetUsedSlots(),
				(unsigned)PARAM_SLOT_COUNT, (unsigned)(status >> 8));
		for (uint8_t i = 0; i < PRESS_CTRL_CH_NUM; i++) {
			Shell_Print("\r\n>> Ch-%d: Kp %u.%02u, Ki %u.%02u, FF %u",
					i,
					rec->kp_x100[i] / 100U, rec->kp_x100[i] % 100U,
					rec->ki_x100[i] / 100U, rec->ki_x100[i] % 100U,
					rec->ff[i]);
		}
		break;
	}

	case 's':
	case 'S':
		/* Only raises a request: the pressure task owns the flash. */
		ParamStore_RequestSave();
		Shell_Print("\r\n>> Save requested");
		break;

	case 'd':
	case 'D':
		/* Goes through the normal command path, so the debounced save picks it
		 * up like any change from the HMI. */
		PressCtrl_SetPI(PRESS_CH_ALL,
				(uint16_t)(PRESS_CTRL_DEFAULT_KP * 100.0f),
				(uint16_t)(PRESS_CTRL_DEFAULT_KI * 100.0f),
				(uint16_t)PRESS_CTRL_DEFAULT_FEEDFORWARD);
		Shell_Print("\r\n>> Defaults applied to all channels");
		break;

	case 'e':
	case 'E':
		ParamStore_RequestErase();
		Shell_Print("\r\n>> Erase requested, defaults will be used after reset");
		break;

	default:
		Shell_Print("\r\n>> %s", paramCmdHelp);
		break;
	}
}

static const DebugCommand_t paramCmd = {"param", paramCmdHelp, paramCommand};

static const DebugCommand_t mbCmd = {"mb", mbCmdHelp, MbDiagCommand};

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

	ret = Shell_RegisterCommand(&mbCmd);
	if (ret) {
		LOG_INFO("Register mb command OK");
	} else {
		LOG_WARNING("Register mb command FAILED");
	}

	ret = Shell_RegisterCommand(&paramCmd);
	if (ret) {
		LOG_INFO("Register param command OK");
	} else {
		LOG_WARNING("Register param command FAILED");
	}
}


