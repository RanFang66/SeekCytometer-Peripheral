/*
 * sample_control.h
 *
 *  Created on: 2025年11月26日
 *      Author: ranfa
 */

#ifndef INC_SAMPLE_CONTROL_H_
#define INC_SAMPLE_CONTROL_H_

#include "dac8568.h"


//#define CHANNEL_NUM (8)	// 8 sample channels
//#define SAMPLE_UPDATE_GAIN 		1
//#define SAMPLE_UPDATE_REF 		2
//#define SAMPLE_UPDATE_GAIN_REF	3


#define SAMPLE_CMD_QUEUE_SIZE 4
typedef enum {
	SAMPLE_CH1 = 0,
	SAMPLE_CH2,
	SAMPLE_CH3,
	SAMPLE_CH4,
	SAMPLE_CH5,
	SAMPLE_CH6,
	SAMPLE_CH7,
	SAMPLE_CH8,
	CHANNEL_NUM,
} sample_ch_t;

typedef enum {
	SAMPLE_RESET_GAIN_REF = 0,
	SAMPLE_UPDATE_GAIN,
	SAMPLE_UPDATE_REF,
	SAMPLE_UPDATE_GAIN_REF,
} SampleCmdType_t;


typedef struct {
	SampleCmdType_t cmdType;
	uint8_t 	enCh;
	uint16_t 	*gains;
	uint16_t 	*refs;
} SampleCmd_t;


typedef struct {
	dac8568_channel_t gainCh;
	dac8568_channel_t refCh;
	uint16_t gain;
	uint16_t ref;
} sample_para_t;

//typedef union {
//	uint16_t ctrlWord;
//	struct BITS {
//		uint16_t cmdType : 8;
//		uint16_t chEn : 8;
//	}ctrlBits;;
//} sample_cmd_t;

HAL_StatusTypeDef SampleCtrl_Init();

HAL_StatusTypeDef SampleCtrl_SetChGain(sample_ch_t channel, uint16_t gain);
HAL_StatusTypeDef SampleCtrl_SetChGainAndUpdate(sample_ch_t channel, uint16_t gain);
HAL_StatusTypeDef SampleCtrl_UpdateAllGain();
HAL_StatusTypeDef SampleCtrl_SetAllGainAndUpdate(uint16_t gains[CHANNEL_NUM]);
HAL_StatusTypeDef SampleCtrl_SetAllSameGain(uint16_t gain);

HAL_StatusTypeDef SampleCtrl_SetChRef(sample_ch_t channel, uint16_t ref);
HAL_StatusTypeDef SampleCtrl_SetChRefAndUpdate(sample_ch_t channel, uint16_t ref);
HAL_StatusTypeDef SampleCtrl_UpdateAllRef();
HAL_StatusTypeDef SampleCtrl_SetAllRefAndUpdate(uint16_t refs[CHANNEL_NUM]);
HAL_StatusTypeDef SampleCtrl_SetAllSameRef(uint16_t ref);



HAL_StatusTypeDef SampleCtrl_SetChGainRef(sample_ch_t channel, uint16_t gain, uint16_t ref);
HAL_StatusTypeDef SampleCtrl_SetChGainRefAndUpdate(sample_ch_t channel, uint16_t gain, uint16_t ref);

uint16_t SampleCtrl_GetChGain(sample_ch_t ch);
uint16_t SampleCtrl_GetChRef(sample_ch_t ch);
void SampleCtrl_GetAll(uint16_t gains[CHANNEL_NUM], uint16_t refs[CHANNEL_NUM]);

void SampleCtrl_StartTask(void);
void SampleCtrl_Reset();
void SampleCtrl_UpdateGains(uint8_t enCh, uint16_t *gains);
void SampleCtrl_UpdateRefs(uint8_t enCh, uint16_t *refs);
void SampleCtrl_UpdateGainRef(uint8_t ench , uint16_t *gains, uint16_t *refs);


#endif /* INC_SAMPLE_CONTROL_H_ */
