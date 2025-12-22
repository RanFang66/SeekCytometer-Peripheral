#ifndef __PZT_QUANTITY_CHECK_H
#define __PZT_QUANTITY_CHECK_H
#include "main.h"
#include "adc.h"


typedef struct {
	uint32_t sum;
	uint16_t count;
	uint16_t max;
	uint16_t min;
	uint16_t value;
	uint16_t ave;
	uint16_t max_err;
} pztq_adc_info;

typedef enum {
	PRP_INIT = 0,
	PRP_CHARGE,
	PRP_BREAK,
	PRP_TEST,
	PRP_DONE
}pztq_rc_phase;

typedef struct {
	pztq_rc_phase phase;
	uint16_t adc_max;
	uint16_t adc_trigger_start;
	uint16_t adc_trigger_stop;
	uint16_t result;
	float udt_sum;
}pztq_rc_runtime;


typedef struct _pztq_check_info{
	ADC_HandleTypeDef *adc_handle;
	void (*adc_callback)(struct _pztq_check_info* qcheck);
	GPIO_TypeDef* port_sw;
	uint16_t pin_sw;
	uint16_t adc_threshold;
	uint16_t adc_baseline;
	uint16_t adc_value;
	uint16_t adc_value_lft;
//	uint16_t adc_buffer_cnt;
//	uint16_t adc_buffer[256];
	float dt;
	float sum_udt_d_rc;
	uint16_t rc;
	int16_t dF;
	int16_t trigger_dF;
	uint32_t timestamp;
	pztq_adc_info adc_info[3];
	uint8_t adc_index;
	pztq_rc_runtime rc_runtime;
} PZTQ_CheckInfo;

extern PZTQ_CheckInfo pztq_check1, pztq_check2;
uint8_t pztq_check(PZTQ_CheckInfo * qcheck);
void pztq_check_start(PZTQ_CheckInfo *info);
void pztq_check_stop(PZTQ_CheckInfo *info);
void pzt_q_set_trigger_dF(PZTQ_CheckInfo * qcheck, int16_t df);
void pztq_check_reset(PZTQ_CheckInfo * qcheck);

void pzt_q_check_baseline_init(PZTQ_CheckInfo * qcheck);
uint8_t pzt_q_check_baseline(PZTQ_CheckInfo * qcheck);

void pzt_q_check_rc_init(PZTQ_CheckInfo * qcheck);
uint8_t pzt_q_check_rc(PZTQ_CheckInfo * qcheck);
#endif
