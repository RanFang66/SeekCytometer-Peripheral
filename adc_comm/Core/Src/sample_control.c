/*
 * sample_control.c
 *
 *  Created on: 2025年11月26日
 *      Author: ranfa
 */


#include "sample_control.h"
#include "spi.h"
#include "cmsis_os2.h"
#include "debug_shell.h"

// Hardware port definition
#define GAIN_CTRL_SPI	hspi1
#define REF_CTRL_SPI	hspi1

#define GAIN_CTRL_SYNC_GPIO		GPIOA
#define GAIN_CTRL_SYNC_PIN		GPIO_PIN_4
#define REF_CTRL_SYNC_GPIO		GPIOC
#define REF_CTRL_SYNC_PIN		GPIO_PIN_4

// Default gain and reference definition
#define GAIN_DEFAULT_VALUE 		1
#define REF_DEFAULT_VALUE		32768


// DAC8568 Instance
static dac8568_t gainDAC;
static dac8568_t refDAC;


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

static const dac8568_channel_t REFS_CH_LIST[CHANNEL_NUM] = {
		DAC8568_CH_B,
		DAC8568_CH_D,
		DAC8568_CH_F,
		DAC8568_CH_H,
		DAC8568_CH_G,
		DAC8568_CH_E,
		DAC8568_CH_C,
		DAC8568_CH_A
};



HAL_StatusTypeDef SampleCtrl_Init()
{
	HAL_StatusTypeDef st = HAL_OK;

	for (sample_ch_t ch = SAMPLE_CH1; ch < CHANNEL_NUM; ++ch) {
		samplePara[ch].gainCh = GAINS_CH_LIST[ch];
		samplePara[ch].refCh = REFS_CH_LIST[ch];
		samplePara[ch].gain = GAIN_DEFAULT_VALUE;
		samplePara[ch].ref = REF_DEFAULT_VALUE;
	}


	st = DAC8568_Init(&gainDAC, &GAIN_CTRL_SPI, GAIN_CTRL_SYNC_GPIO, GAIN_CTRL_SYNC_PIN, false, 3.3);
	if (st != HAL_OK) {
		return st;
	}

	st = DAC8568_Init(&refDAC, &REF_CTRL_SPI, REF_CTRL_SYNC_GPIO, REF_CTRL_SYNC_PIN, false, 3.3);
	if (st != HAL_OK) {
		return st;
	}


	// Initialize All gain and reference to default
	st = DAC8568_BroadcastWriteUpdate(&gainDAC, GAIN_DEFAULT_VALUE);
	if (st != HAL_OK) {
		return st;
	}

	st = DAC8568_BroadcastWriteUpdate(&refDAC, REF_DEFAULT_VALUE);
	if (st != HAL_OK) {
		return st;
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
	return DAC8568_WriteInputOnly(&refDAC, samplePara[channel].refCh, ref);
}

HAL_StatusTypeDef SampleCtrl_SetChRefAndUpdate(sample_ch_t channel, uint16_t ref)
{
	if (channel >= CHANNEL_NUM) {
		return HAL_ERROR;
	}
	samplePara[channel].ref = ref;
	return DAC8568_WriteUpdate(&refDAC, samplePara[channel].refCh, ref);
}


HAL_StatusTypeDef SampleCtrl_UpdateAllRef()
{
	return DAC8568_UpdateAll(&refDAC);
}

HAL_StatusTypeDef SampleCtrl_SetAllRefAndUpdate(uint16_t refs[CHANNEL_NUM])
{
	if (!refs) return HAL_ERROR;

	HAL_StatusTypeDef st = HAL_OK;

	for (sample_ch_t ch = SAMPLE_CH1; ch < CHANNEL_NUM; ++ch) {
		samplePara[ch].ref = refs[ch];
		st = DAC8568_WriteInputOnly(&refDAC, samplePara[ch].refCh, refs[ch]);
		if (st != HAL_OK) {
			return st;
		}
	}

	return DAC8568_UpdateAll(&refDAC);
}

HAL_StatusTypeDef SampleCtrl_SetAllSameRef(uint16_t ref)
{
	for (sample_ch_t ch = SAMPLE_CH1; ch < CHANNEL_NUM; ++ch) {
		samplePara[ch].ref = ref;
	}
	return DAC8568_BroadcastWriteUpdate(&refDAC, ref);
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
	st = DAC8568_WriteInputOnly(&refDAC, samplePara[channel].refCh, ref);
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
	st = DAC8568_WriteUpdate(&refDAC, samplePara[channel].refCh, ref);
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
	DAC8568_BroadcastWriteUpdate(&refDAC, REF_DEFAULT_VALUE);

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

