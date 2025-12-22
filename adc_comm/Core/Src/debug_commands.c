/*
 * debug_commands.c
 *
 *  Created on: 2025年11月27日
 *      Author: ranfa
 */

#include "debug_shell.h"
#include <stdlib.h>
#include "sample_control.h"

static const char sampleCmdHelp[] = "Configure sample gain and reference, Use sample -g/r/i id val";
static void SampleControlCommand(int argc, char *argv[])
{
	if (argc < 3) {
		Shell_Print("\r\n>> %s", sampleCmdHelp);
		return;
	}

	uint8_t id = 0;
	uint16_t gainVal = 0;
	uint16_t refVal = 0;

	if (argv[2][0] == 'a' || argv[2][0] == 'A') {
		id = 0xFF;
	} else if (argv[2][0] >= '1' && argv[2][0] <= '8') {
		id = argv[2][0] - '1';
	} else {
		Shell_Print("\r\n Wrong channel id, 'a' for all channel or '1'~'8' for single channel");
		return;
	}

	if (strcmp(argv[1], "-i") == 0 || strcmp(argv[1], "-info") == 0) {
		if (id == 0xFF) {
			uint16_t gains[CHANNEL_NUM];
			uint16_t refs[CHANNEL_NUM];
			SampleCtrl_GetAll(gains, refs);
			Shell_Print("\r\n Channel gains and refs: 1)%d %d, 2)%d %d, 3)%d %d, 4)%d %d, 5)%d %d, 6)%d %d 7)%d %d, 8)%d %d",
					gains[0], refs[0], gains[1], refs[1], gains[2], refs[2], gains[3], refs[3], gains[4], refs[4],
					gains[5], refs[5], gains[6], refs[6], gains[7], refs[7]);
		} else {
			uint16_t gain = SampleCtrl_GetChGain((sample_ch_t)id);
			uint16_t ref = SampleCtrl_GetChRef((sample_ch_t)id);
			Shell_Print("channel %d gain: %d, ref: %d", id+1, gain, ref);
		}
	} else if (strcmp(argv[1], "-g") == 0 || strcmp(argv[1], "-gain") == 0) {
		if (argc < 4) {
			Shell_Print("\r\n>> %s", sampleCmdHelp);
			return;
		}
		gainVal = atoi(argv[3]);
		if (id == 0xFF) {
			SampleCtrl_SetAllSameGain(gainVal);
		} else {
			SampleCtrl_SetChGainAndUpdate((sample_ch_t)id, gainVal);
		}
	} else if (strcmp(argv[1], "-r") == 0 || strcmp(argv[1], "-ref") == 0) {
		if (argc < 4) {
			Shell_Print("\r\n>> %s", sampleCmdHelp);
			return;
		}
		refVal = atoi(argv[3]);
		if (id == 0xFF) {
			SampleCtrl_SetAllSameRef(refVal);
		} else {
			SampleCtrl_SetChRefAndUpdate((sample_ch_t)id, refVal);
		}
	} else {
		Shell_Print("\r\n>> %s", sampleCmdHelp);
	}
}

static DebugCommand_t sampleCmd = {"sample", "Configure sample gain and reference, Use sample -g/r id val", SampleControlCommand};


void registerDebugCommands(void)
{
	bool ret = Shell_RegisterCommand(&sampleCmd);
	if (ret) {
		LOG_INFO("Register sample command OK");
	} else {
		LOG_WARNING("Register sample command FAILED");
	}
}
