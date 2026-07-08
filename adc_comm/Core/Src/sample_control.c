/*
 * sample_control.c
 *
 *  Created on: 2025年11月26日
 *      Author: ranfa
 */


#include "sample_control.h"
#include "spi.h"
#include "gpio.h"
#include "main.h"
#include "cmsis_os2.h"
#include "debug_shell.h"

// Hardware port definition
#define GAIN_CTRL_SPI	hspi1
#define REF_CTRL_SPI	hspi1

#define GAIN_CTRL_SYNC_GPIO		SYNC2_GPIO_Port
#define GAIN_CTRL_SYNC_PIN		SYNC2_Pin

// Two AD5724R share hspi1 with the gain DAC8568; each has its own chip select.
// ref_cs1 (PB0) drives channels 1-4, ref_cs2 (PB1) drives channels 5-8.
#define REF_CTRL_CS1_GPIO		ref_cs1_GPIO_Port
#define REF_CTRL_CS1_PIN		ref_cs1_Pin
#define REF_CTRL_CS2_GPIO		ref_cs2_GPIO_Port
#define REF_CTRL_CS2_PIN		ref_cs2_Pin

#define REF_CHIP_NUM			2

// AD5724R uses an internal 2.5 V reference with the +-5 V bipolar range.
#define REF_OUTPUT_RANGE		AD5724R_RANGE_BI_5V
#define REF_USE_INTERNAL_REF	true

// Default gain and reference definition
#define GAIN_DEFAULT_VALUE 		1
// Bipolar offset-binary midscale (0x8000) corresponds to 0 V output.
#define REF_DEFAULT_VALUE		32768


// Gain DAC8568 instance
static dac8568_t gainDAC;
// Reference AD5724R instances: [0] -> ref_cs1 (CH1-4), [1] -> ref_cs2 (CH5-8)
static ad5724r_t refDAC[REF_CHIP_NUM];


/* Variables for modbus communication */
// Current Gain and Reference values
sample_para_t samplePara[CHANNEL_NUM];

static osThreadId_t sampleCtrlHandle = NULL;
static osMessageQueueId_t sampleCmdQueue = NULL;

static const dac8568_channel_t GAINS_CH_LIST[CHANNEL_NUM] = {
		DAC8568_CH_B,
		DAC8568_CH_D,
		DAC8568_CH_F,
		DAC8568_CH_H,
		DAC8568_CH_G,
		DAC8568_CH_E,
		DAC8568_CH_C,
		DAC8568_CH_A
};

// Reference channel map: which AD5724R chip and channel drives each sample
// channel. Chip 0 (ref_cs1) serves CH1-4, chip 1 (ref_cs2) serves CH5-8.
// Adjust the {chip, channel} pairs here to match the PCB routing if needed.
typedef struct {
	uint8_t           chip;
	ad5724r_channel_t ch;
} ref_map_t;

static const ref_map_t REFS_MAP[CHANNEL_NUM] = {
		{0, AD5724R_CH_A},
		{0, AD5724R_CH_B},
		{0, AD5724R_CH_C},
		{0, AD5724R_CH_D},
		{1, AD5724R_CH_A},
		{1, AD5724R_CH_B},
		{1, AD5724R_CH_C},
		{1, AD5724R_CH_D},
};



HAL_StatusTypeDef SampleCtrl_Init()
{
	HAL_StatusTypeDef st = HAL_OK;

	for (sample_ch_t ch = SAMPLE_CH1; ch < CHANNEL_NUM; ++ch) {
		samplePara[ch].gainCh = GAINS_CH_LIST[ch];
		samplePara[ch].refChip = REFS_MAP[ch].chip;
		samplePara[ch].refCh = REFS_MAP[ch].ch;
		samplePara[ch].gain = GAIN_DEFAULT_VALUE;
		samplePara[ch].ref = REF_DEFAULT_VALUE;
	}


	st = DAC8568_Init(&gainDAC, &GAIN_CTRL_SPI, GAIN_CTRL_SYNC_GPIO, GAIN_CTRL_SYNC_PIN, false, 3.3);
	if (st != HAL_OK) {
		return st;
	}

	st = AD5724R_Init(&refDAC[0], &REF_CTRL_SPI, REF_CTRL_CS1_GPIO, REF_CTRL_CS1_PIN,
			REF_OUTPUT_RANGE, REF_USE_INTERNAL_REF);
	if (st != HAL_OK) {
		return st;
	}

	st = AD5724R_Init(&refDAC[1], &REF_CTRL_SPI, REF_CTRL_CS2_GPIO, REF_CTRL_CS2_PIN,
			REF_OUTPUT_RANGE, REF_USE_INTERNAL_REF);
	if (st != HAL_OK) {
		return st;
	}


	// Initialize All gain and reference to default
	st = DAC8568_BroadcastWriteUpdate(&gainDAC, GAIN_DEFAULT_VALUE);
	if (st != HAL_OK) {
		return st;
	}

	for (uint8_t chip = 0; chip < REF_CHIP_NUM; ++chip) {
		st = AD5724R_BroadcastWriteUpdate(&refDAC[chip], REF_DEFAULT_VALUE);
		if (st != HAL_OK) {
			return st;
		}
	}

	return HAL_OK;
}

HAL_StatusTypeDef SampleCtrl_SetChGain(sample_ch_t channel, uint16_t gain)
{
	if (channel >= CHANNEL_NUM) {
		return HAL_ERROR;
	}

	samplePara[channel].gain = gain;
	return DAC8568_WriteInputOnly(&gainDAC, samplePara[channel].gainCh, gain);
}


HAL_StatusTypeDef SampleCtrl_SetChGainAndUpdate(sample_ch_t channel, uint16_t gain)
{
	if (channel >= CHANNEL_NUM) {
		return HAL_ERROR;
	}
	samplePara[channel].gain = gain;
	return DAC8568_WriteUpdate(&gainDAC, samplePara[channel].gainCh, gain);
}



HAL_StatusTypeDef SampleCtrl_UpdateAllGain()
{
	return DAC8568_UpdateAll(&gainDAC);
}

HAL_StatusTypeDef SampleCtrl_SetAllGainAndUpdate(uint16_t gains[CHANNEL_NUM])
{
	if (!gains) return HAL_ERROR;

	HAL_StatusTypeDef st = HAL_OK;

	for (sample_ch_t ch = SAMPLE_CH1; ch < CHANNEL_NUM; ++ch) {
		samplePara[ch].gain = gains[ch];
		st = DAC8568_WriteInputOnly(&gainDAC, samplePara[ch].gainCh, gains[ch]);
		if (st != HAL_OK) {
			return st;
		}
	}

	return DAC8568_UpdateAll(&gainDAC);
}

HAL_StatusTypeDef SampleCtrl_SetAllSameGain(uint16_t gain)
{
	for (sample_ch_t ch = SAMPLE_CH1; ch < CHANNEL_NUM; ++ch) {
		samplePara[ch].gain = gain;
	}
	return DAC8568_BroadcastWriteUpdate(&gainDAC, gain);
}


HAL_StatusTypeDef SampleCtrl_SetChRef(sample_ch_t channel, uint16_t ref)
{
	if (channel >= CHANNEL_NUM) {
		return HAL_ERROR;
	}

	samplePara[channel].ref = ref;
	return AD5724R_WriteInputOnly(&refDAC[samplePara[channel].refChip],
			samplePara[channel].refCh, ref);
}

HAL_StatusTypeDef SampleCtrl_SetChRefAndUpdate(sample_ch_t channel, uint16_t ref)
{
	if (channel >= CHANNEL_NUM) {
		return HAL_ERROR;
	}
	samplePara[channel].ref = ref;
	return AD5724R_WriteUpdate(&refDAC[samplePara[channel].refChip],
			samplePara[channel].refCh, ref);
}


HAL_StatusTypeDef SampleCtrl_UpdateAllRef()
{
	HAL_StatusTypeDef st = HAL_OK;
	for (uint8_t chip = 0; chip < REF_CHIP_NUM; ++chip) {
		st = AD5724R_Update(&refDAC[chip]);
		if (st != HAL_OK) {
			return st;
		}
	}
	return st;
}

HAL_StatusTypeDef SampleCtrl_SetAllRefAndUpdate(uint16_t refs[CHANNEL_NUM])
{
	if (!refs) return HAL_ERROR;

	HAL_StatusTypeDef st = HAL_OK;

	for (sample_ch_t ch = SAMPLE_CH1; ch < CHANNEL_NUM; ++ch) {
		samplePara[ch].ref = refs[ch];
		st = AD5724R_WriteInputOnly(&refDAC[samplePara[ch].refChip],
				samplePara[ch].refCh, refs[ch]);
		if (st != HAL_OK) {
			return st;
		}
	}

	return SampleCtrl_UpdateAllRef();
}

HAL_StatusTypeDef SampleCtrl_SetAllSameRef(uint16_t ref)
{
	HAL_StatusTypeDef st = HAL_OK;
	for (sample_ch_t ch = SAMPLE_CH1; ch < CHANNEL_NUM; ++ch) {
		samplePara[ch].ref = ref;
	}
	for (uint8_t chip = 0; chip < REF_CHIP_NUM; ++chip) {
		st = AD5724R_BroadcastWriteUpdate(&refDAC[chip], ref);
		if (st != HAL_OK) {
			return st;
		}
	}
	return st;
}


HAL_StatusTypeDef SampleCtrl_SetChGainRef(sample_ch_t channel, uint16_t gain, uint16_t ref)
{
	if (channel >= CHANNEL_NUM) {
		return HAL_ERROR;
	}
	HAL_StatusTypeDef st = HAL_OK;
	samplePara[channel].gain = gain;
	samplePara[channel].ref = ref;
	st = DAC8568_WriteInputOnly(&gainDAC, samplePara[channel].gainCh, gain);
	if (st != HAL_OK) {
		return st;
	}
	st = AD5724R_WriteInputOnly(&refDAC[samplePara[channel].refChip],
			samplePara[channel].refCh, ref);
	return st;
}


HAL_StatusTypeDef SampleCtrl_SetChGainRefAndUpdate(sample_ch_t channel, uint16_t gain, uint16_t ref)
{
	if (channel >= CHANNEL_NUM) {
		return HAL_ERROR;
	}
	HAL_StatusTypeDef st = HAL_OK;
	samplePara[channel].gain = gain;
	samplePara[channel].ref = ref;
	st = DAC8568_WriteUpdate(&gainDAC, samplePara[channel].gainCh, gain);
	if (st != HAL_OK) {
		return st;
	}
	st = AD5724R_WriteUpdate(&refDAC[samplePara[channel].refChip],
			samplePara[channel].refCh, ref);
	return st;
}


uint16_t SampleCtrl_GetChGain(sample_ch_t ch)
{
	return samplePara[ch].gain;
}

uint16_t SampleCtrl_GetChRef(sample_ch_t ch)
{
	return samplePara[ch].ref;
}

void SampleCtrl_GetAll(uint16_t gains[CHANNEL_NUM], uint16_t refs[CHANNEL_NUM])
{
	for (sample_ch_t ch = SAMPLE_CH1; ch < CHANNEL_NUM; ++ch) {
		gains[ch] = samplePara[ch].gain;
		refs[ch] = samplePara[ch].ref;
	}
}

static void resetGainRefs()
{
	DAC8568_BroadcastWriteUpdate(&gainDAC, GAIN_DEFAULT_VALUE);
	for (uint8_t chip = 0; chip < REF_CHIP_NUM; ++chip) {
		AD5724R_BroadcastWriteUpdate(&refDAC[chip], REF_DEFAULT_VALUE);
	}

	for (sample_ch_t ch = SAMPLE_CH1; ch < CHANNEL_NUM; ++ch) {
		samplePara[ch].gain = GAIN_DEFAULT_VALUE;
		samplePara[ch].ref = REF_DEFAULT_VALUE;
	}
}

static void updateGain(uint8_t chEn, uint16_t *gainSet)
{
	if (chEn == 0) {
		return;
	}

	if (chEn == 0xFF) {
		SampleCtrl_SetAllGainAndUpdate(gainSet);
	} else {
		for (int i = 0; i < 8; i++) {
			if ((chEn >> i) & 0x01) {
				SampleCtrl_SetChGainAndUpdate((sample_ch_t)i, gainSet[i]);
			}
		}
	}
}

static void updateRef(uint8_t chEn, uint16_t *refSet)
{
	if (chEn == 0) {
		return;
	}

	if (chEn == 0xFF) {
		SampleCtrl_SetAllRefAndUpdate(refSet);
	} else {
		for (int i = 0; i < 8; i++) {
			if ((chEn >> i) & 0x01) {
				SampleCtrl_SetChRefAndUpdate((sample_ch_t)i, refSet[i]);
			}
		}
	}
}

static void updateGainAndRef(uint8_t chEn, uint16_t *gainSet, uint16_t *refSet)
{
	if (chEn == 0) {
		return;
	}

	if (chEn == 0xFF) {
		SampleCtrl_SetAllGainAndUpdate(gainSet);
		SampleCtrl_SetAllRefAndUpdate(refSet);
	} else {
		for (int i = 0; i < 8; i++) {
			if ((chEn >> i) & 0x01) {
				SampleCtrl_SetChGainAndUpdate((sample_ch_t)i, gainSet[i]);
				SampleCtrl_SetChRefAndUpdate((sample_ch_t)i, refSet[i]);
			}
		}
	}
}

static void SampleCtrl_Task(void *arg)
{
	for (;;) {
		SampleCmd_t cmd;
		if (osMessageQueueGet(sampleCmdQueue, &cmd, NULL, osWaitForever) == osOK) {
			switch (cmd.cmdType) {
			case SAMPLE_RESET_GAIN_REF:
				resetGainRefs();
				break;
			case SAMPLE_UPDATE_GAIN:
				updateGain(cmd.enCh, cmd.gains);
				break;
			case SAMPLE_UPDATE_REF:
				updateRef(cmd.enCh, cmd.refs);
				break;
			case SAMPLE_UPDATE_GAIN_REF:
				updateGainAndRef(cmd.enCh, cmd.gains, cmd.refs);
				break;
			default:
				break;
			}
		}
	}
}

void SampleCtrl_StartTask(void)
{
	sampleCmdQueue = osMessageQueueNew(SAMPLE_CMD_QUEUE_SIZE, sizeof(SampleCmd_t), NULL);
	if (sampleCmdQueue == NULL) {
		LOG_ERROR("Create sample control command queue FAILED");
	}

    const osThreadAttr_t task_attributes = {
        .name = "SampleControl",
        .stack_size = 256 * 4,
        .priority = (osPriority_t) osPriorityNormal,
    };
    // FIX: Use the dot operator to access taskHandle
    sampleCtrlHandle = osThreadNew(SampleCtrl_Task, NULL, &task_attributes);
    if (sampleCtrlHandle) {
    	LOG_INFO("Create sample control task OK");
    } else {
    	LOG_ERROR("Create sample control task FAILED");
    }
}


void SampleCtrl_Reset()
{
	SampleCmd_t cmd = {.cmdType = SAMPLE_RESET_GAIN_REF};
	if (osMessageQueuePut(sampleCmdQueue, &cmd, 0, 100) != osOK) {
		LOG_WARNING("Send sample reset command FAILED!");
	}
}


void SampleCtrl_UpdateGains(uint8_t enCh, uint16_t *gains)
{
	SampleCmd_t cmd = {.cmdType = SAMPLE_UPDATE_GAIN, .enCh = enCh, .gains = gains};
	if (osMessageQueuePut(sampleCmdQueue, &cmd, 0, 100) != osOK) {
		LOG_WARNING("Send sample update gains command FAILED!");
	}
}
void SampleCtrl_UpdateRefs(uint8_t enCh, uint16_t *refs)
{
	SampleCmd_t cmd = {.cmdType = SAMPLE_UPDATE_REF, .enCh = enCh, .refs = refs};
	if (osMessageQueuePut(sampleCmdQueue, &cmd, 0, 100) != osOK) {
		LOG_WARNING("Send sample update references command FAILED!");
	}
}


void SampleCtrl_UpdateGainRef(uint8_t enCh , uint16_t *gains, uint16_t *refs)
{
	SampleCmd_t cmd = {.cmdType = SAMPLE_UPDATE_GAIN_REF, .enCh = enCh, .gains = gains, .refs = refs};
	if (osMessageQueuePut(sampleCmdQueue, &cmd, 0, 100) != osOK) {
		LOG_WARNING("Send sample update gains and references command FAILED!");
	}
}

