#include "pzt_quantity_check.h"
#include "time.h"
#include "interface.h"
#include "string.h"
#include "stdio.h"
#include "step_motor_driver.h"
#include "math.h"
#include "stdlib.h"

#define SW1_H HAL_GPIO_WritePin(SW1_GPIO_Port, SW1_Pin, GPIO_PIN_SET)
#define SW1_L HAL_GPIO_WritePin(SW1_GPIO_Port, SW1_Pin, GPIO_PIN_RESET)
#define SW2_H HAL_GPIO_WritePin(SW2_GPIO_Port, SW2_Pin, GPIO_PIN_SET)
#define SW2_L HAL_GPIO_WritePin(SW2_GPIO_Port, SW2_Pin, GPIO_PIN_RESET)
#define ADC_SAMPLE_TIME (0.04686f)  // 单位ms


static void pztq_adc_callback(PZTQ_CheckInfo * qcheck)
{
	qcheck->dt += ADC_SAMPLE_TIME;
	qcheck->adc_value = qcheck->adc_handle->Instance->DR;
	qcheck->adc_value_lft += (qcheck->adc_value - qcheck->adc_value_lft) * 0.1f;
}

PZTQ_CheckInfo pztq_check1 = {
		.adc_handle = &hadc1,
		.adc_callback = pztq_adc_callback,
		.port_sw = SW1_GPIO_Port,
		.pin_sw = SW1_Pin,
		.adc_baseline = 0,
		.adc_threshold = 30,
		.rc = 1200,
		.trigger_dF = 200
};

PZTQ_CheckInfo pztq_check2 = {
		.adc_handle = &hadc2,
		.adc_callback = pztq_adc_callback,
		.port_sw = SW2_GPIO_Port,
		.pin_sw = SW2_Pin,
		.adc_baseline = 0,
		.adc_threshold = 30,
		.rc = 1200,
		.trigger_dF = 200
};

uint8_t pztq_check(PZTQ_CheckInfo * qcheck)
{
	__disable_irq();
	if (abs (qcheck->adc_value - qcheck->adc_baseline) <= qcheck->adc_threshold) {
		qcheck->dt = 0.f;
		qcheck->sum_udt_d_rc = 0;
		qcheck->dF = 0;
	} else {
		qcheck->sum_udt_d_rc += (qcheck->adc_value - qcheck->adc_baseline) * qcheck->dt / qcheck->rc;
		qcheck->dt = 0.f;
		qcheck->dF = qcheck->sum_udt_d_rc + (qcheck->adc_value_lft - qcheck->adc_baseline);
	}
	__enable_irq();

	if (qcheck->trigger_dF > 0)
		return (qcheck->dF > qcheck->trigger_dF) ? 1 : 0;
	else
		return (qcheck->dF < qcheck->trigger_dF) ? 1 : 0;
}


void pztq_check_start(PZTQ_CheckInfo *info)
{
	HAL_GPIO_WritePin(info->port_sw, info->pin_sw, GPIO_PIN_SET);
	HAL_ADC_Start_IT(info->adc_handle);
}

void pztq_check_reset(PZTQ_CheckInfo * qcheck)
{
	__disable_irq();
	qcheck->dt = 0;
	qcheck->sum_udt_d_rc = 0;
	qcheck->dF = 0;
	qcheck->adc_value_lft = qcheck->adc_baseline;
	__enable_irq();
}
void pztq_check_stop(PZTQ_CheckInfo *info)
{
	HAL_ADC_Stop_IT(info->adc_handle);
	HAL_GPIO_WritePin(info->port_sw, info->pin_sw, GPIO_PIN_RESET);
}

//static uint16_t pzt_get_adc_value(void)
//{
//	uint32_t adc = 0, i;
//	HAL_ADC_Start_DMA(&hadc1, (uint32_t *)pzt_check_adc_buffer, 100 * 2);
//	while(hadc1.DMA_Handle->Instance->CR & DMA_SxCR_EN);
//	HAL_ADC_Stop_DMA(&hadc1);
//
//	for (i = 0, adc = 0; i < 100; i++) {
//		adc += pzt_check_adc_buffer[i][1];
//	}
//	return adc / 100;
//}



/* PZT 基于电荷量方案的校准功能1
 * 找到基准线和信号噪声阈值
 * 		电机等一切都停止时，检查ADC值
 * 		当平均值不再单调变化时，记录其均值为baseline
 * 		信号噪声最大值 为阈值
 * */

//static void pzt_q_read_baseline(pztq_adc_info *info, PZTQ_CheckInfo * qcheck)
//{
//	for (memset(info, 0, sizeof(pztq_adc_info)), info->min = 4095; info->count < 100; info->count++) {
//		info->value = qcheck->adc_value;
//		info->sum += info->value;
//		info->count ++;
//		if (info->max < info->value)
//			info->max = info->value;
//		if (info->min > info->value)
//			info->min = info->value;
//		HAL_Delay(1);
//	}
//	info->ave = info->sum / info->count;
//	info->max_err = (info->ave - info->min) > (info->max - info->ave) ?
//					(info->ave - info->min) : (info->max - info->ave);
//}

//void pzt_q_check_baseline(PZTQ_CheckInfo * qcheck)
//{
//	uint8_t idx = 0;
//	pztq_adc_info adc_info[3];
//	SW2_H;
//	pzt_q_read_baseline(&adc_info[0], qcheck);
//	HAL_Delay(100);
//	pzt_q_read_baseline(&adc_info[1], qcheck);
//	HAL_Delay(100);
//	pzt_q_read_baseline(&adc_info[2], qcheck);
//
//	while ((adc_info[2].ave - adc_info[1].ave) * (adc_info[1].ave - adc_info[0].ave) > 0) {
//		HAL_Delay(100);
//		pzt_q_read_baseline(&adc_info[idx], qcheck);
//		idx = (idx + 1) % 3;
//	}
//	qcheck->adc_baseline = (adc_info[0].ave + adc_info[1].ave + adc_info[2].ave) / 3;
//	qcheck->adc_threshold = adc_info[0].max_err;
//	if (qcheck->adc_threshold < adc_info[1].max_err)
//		qcheck->adc_threshold = adc_info[1].max_err;
//	if (qcheck->adc_threshold < adc_info[2].max_err)
//		qcheck->adc_threshold = adc_info[2].max_err;
//}

// 碎片化实现
//static pztq_adc_info qcheck_adc_info[3];
//static uint8_t qcheck_adc_idx = 0;
void pzt_q_check_baseline_init(PZTQ_CheckInfo * qcheck)
{
	HAL_GPIO_WritePin(qcheck->port_sw, qcheck->pin_sw, GPIO_PIN_RESET);
	memset(qcheck->adc_info, 0, sizeof(qcheck->adc_info));
	qcheck->adc_index = 0;
	qcheck->adc_info[0].min = 4095;
	qcheck->adc_info[1].min = 4095;
	qcheck->adc_info[2].min = 4095;
	bsp_time_clear(qcheck->timestamp);
}

uint8_t pzt_q_check_baseline(PZTQ_CheckInfo * qcheck)
{
	pztq_adc_info* info = qcheck->adc_info + qcheck->adc_index;
	if (bsp_time_less(qcheck->timestamp, 1))
		return 0;
	bsp_time_clear(qcheck->timestamp);
	if (info->count < 100) {
		info->value = qcheck->adc_value;
		info->sum += info->value;
		info->count ++;
		if (info->max < info->value)
			info->max = info->value;
		if (info->min > info->value)
			info->min = info->value;
		return 0;
	}

	info->ave = info->sum / info->count;
	info->max_err = (info->ave - info->min) > (info->max - info->ave) ?
					(info->ave - info->min) : (info->max - info->ave);

	qcheck->adc_index = (qcheck->adc_index + 1) % 3;
	info = qcheck->adc_info + qcheck->adc_index;
	if (info->count < 100)
		return 0;
	if (qcheck->adc_info[0].ave > 2500 ||
		qcheck->adc_info[1].ave > 2500 ||
		qcheck->adc_info[2].ave > 2500 ) {
		memset(info, 0, sizeof(pztq_adc_info));
		info->min = 4095;
		return 0;
	}

	if ((qcheck->adc_info[qcheck->adc_index].ave - qcheck->adc_info[(qcheck->adc_index + 1) % 3].ave) *
		(qcheck->adc_info[(qcheck->adc_index + 1) % 3].ave - qcheck->adc_info[(qcheck->adc_index + 2) % 3].ave) > 0) {
		memset(info, 0, sizeof(pztq_adc_info));
		info->min = 4095;
		return 0;
	}
	qcheck->adc_baseline = (qcheck->adc_info[0].ave + qcheck->adc_info[1].ave + qcheck->adc_info[2].ave) / 3;
	qcheck->adc_threshold = qcheck->adc_info[0].max_err;
	if (qcheck->adc_threshold < qcheck->adc_info[1].max_err)
		qcheck->adc_threshold = qcheck->adc_info[1].max_err;
	if (qcheck->adc_threshold < qcheck->adc_info[2].max_err)
		qcheck->adc_threshold = qcheck->adc_info[2].max_err;
	return 1;
}

/* PZT 基于电荷量方案的校准功能2
 * 找到电荷稳定时的放电常数rc
 * 通过驱动信号对PZT施加电压，再瞬间切断 等到PZT自身震动平稳后开始测量
 * 释放过程中记录ADC值
 * 取其衰减过程最大值到baseline中 80% - 20%的部分
 * rc = ∑u*dt/(u0-u1)
 * */
//void pzt_q_check_rc(PZTQ_CheckInfo * qcheck)
//{
//	uint16_t adc_max = 0;
//	uint16_t adc_trigger_start = 0;
//	uint16_t adc_trigger_stop = 0;
//	float udt_sum = 0.f;
//	uint16_t adc_value;
//	float dt;
//	SW2_L;
//	HAL_GPIO_WritePin(TIRGGER_GPIO_Port, TIRGGER_Pin, GPIO_PIN_SET);
//	HAL_Delay(1000);
//	SW2_H;
//	HAL_GPIO_WritePin(TIRGGER_GPIO_Port, TIRGGER_Pin, GPIO_PIN_RESET);
//
//	HAL_Delay(50);
//	while (qcheck->adc_value > qcheck->adc_baseline + qcheck->adc_threshold) {
//		__disable_irq();
//		adc_value = qcheck->adc_value;
//		dt = qcheck->dt;
//		qcheck->dt = 0;
//		__enable_irq();
//		if (adc_max < adc_value) {
//			adc_max = adc_value;
//			adc_trigger_start = adc_max - (adc_max - qcheck->adc_baseline) / 5;
//			adc_trigger_stop = qcheck->adc_baseline + (adc_max - qcheck->adc_baseline) / 5;
//		}
//
//		if (adc_value > adc_trigger_start) {
//			udt_sum = 0;
//		} else if (adc_value > adc_trigger_stop) {
//			udt_sum += (adc_value - qcheck->adc_baseline) * dt;
//		} else {
//			qcheck->rc = udt_sum / (adc_trigger_start - adc_trigger_stop);
//			return;
//		}
//		HAL_Delay(1);
//	}
//}

// 碎片化实现
void pzt_q_check_rc_init(PZTQ_CheckInfo * qcheck)
{
	qcheck->rc_runtime.phase = PRP_INIT;
	qcheck->rc_runtime.adc_max = 0;
	qcheck->rc_runtime.adc_trigger_start = 0;
	qcheck->rc_runtime.adc_trigger_stop = 0;
	qcheck->rc_runtime.udt_sum = 0;
	qcheck->rc_runtime.result = 0;
}

uint8_t pzt_q_check_rc(PZTQ_CheckInfo * qcheck)
{

#if 0
	switch (qcheck->rc_runtime.phase) {
	case PRP_INIT:
		HAL_GPIO_WritePin(qcheck->port_sw, qcheck->pin_sw, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(TIRGGER_GPIO_Port, TIRGGER_Pin, GPIO_PIN_SET);
		bsp_time_clear(qcheck->timestamp);
		qcheck->rc_runtime.phase = PRP_CHARGE;
		break;
	case PRP_CHARGE:
		if (bsp_time_more(qcheck->timestamp, 1000)) {
			bsp_time_clear(qcheck->timestamp);
			HAL_GPIO_WritePin(qcheck->port_sw, qcheck->pin_sw, GPIO_PIN_SET);
			HAL_GPIO_WritePin(TIRGGER_GPIO_Port, TIRGGER_Pin, GPIO_PIN_RESET);
			qcheck->rc_runtime.phase = PRP_BREAK;
		}
		break;
	case PRP_BREAK:
		if (bsp_time_more(qcheck->timestamp, 50)) {
			bsp_time_clear(qcheck->timestamp);
			qcheck->rc_runtime.phase = PRP_TEST;
		}
		break;
	case PRP_TEST:
		if (bsp_time_more(qcheck->timestamp, 1)) {
			bsp_time_clear(qcheck->timestamp);
			if (abs(qcheck->adc_value - qcheck->adc_baseline) > qcheck->adc_threshold) {
				uint16_t adc_value;
				float dt;
				__disable_irq();
				adc_value = qcheck->adc_value;
				dt = qcheck->dt;
				qcheck->dt = 0;
				__enable_irq();
				if (qcheck->rc_runtime.adc_max < adc_value) {
					qcheck->rc_runtime.adc_max = adc_value;
					qcheck->rc_runtime.adc_trigger_start = qcheck->rc_runtime.adc_max - (qcheck->rc_runtime.adc_max - qcheck->adc_baseline) / 5;
					qcheck->rc_runtime.adc_trigger_stop = qcheck->adc_baseline + (qcheck->rc_runtime.adc_max - qcheck->adc_baseline) / 5;
				}

				if (adc_value > qcheck->rc_runtime.adc_trigger_start) {
					qcheck->rc_runtime.udt_sum = 0;
				} else if (adc_value > qcheck->rc_runtime.adc_trigger_stop) {
					qcheck->rc_runtime.udt_sum += (adc_value - qcheck->adc_baseline) * dt;
				} else {
					qcheck->rc = qcheck->rc_runtime.udt_sum / (qcheck->rc_runtime.adc_trigger_start - qcheck->rc_runtime.adc_trigger_stop);
					qcheck->rc_runtime.phase = PRP_DONE;
					qcheck->rc_runtime.result = 1;
				}
			} else {
				qcheck->rc_runtime.phase = PRP_DONE;
			}

		}
		break;
	case PRP_DONE:
		break;
	}
	return qcheck->rc_runtime.phase == PRP_DONE ? 1 : 0;

#else
	switch (qcheck->rc_runtime.phase) {
	case PRP_INIT:
		HAL_GPIO_WritePin(qcheck->port_sw, qcheck->pin_sw, GPIO_PIN_SET);
		bsp_time_clear(qcheck->timestamp);
		mcp4728_write_single(1, 1200);
		qcheck->rc_runtime.phase = PRP_CHARGE;
		break;
	case PRP_CHARGE:
		if (bsp_time_more(qcheck->timestamp, 5000)) {
			bsp_time_clear(qcheck->timestamp);
			mcp4728_write_single(1, 120);
			qcheck->rc_runtime.phase = PRP_TEST;
		}
		break;
	case PRP_BREAK:
		if (bsp_time_more(qcheck->timestamp, 50)) {
			bsp_time_clear(qcheck->timestamp);
			qcheck->rc_runtime.phase = PRP_TEST;
		}
		break;
	case PRP_TEST:
		if (bsp_time_more(qcheck->timestamp, 1)) {
			bsp_time_clear(qcheck->timestamp);
			if (abs(qcheck->adc_value - qcheck->adc_baseline) > qcheck->adc_threshold) {
				uint16_t adc_value;
				float dt;
				__disable_irq();
				adc_value = qcheck->adc_value;
				dt = qcheck->dt;
				qcheck->dt = 0;
				__enable_irq();
				if (qcheck->rc_runtime.adc_max < abs(adc_value - qcheck->adc_baseline)) {
					qcheck->rc_runtime.adc_max = abs(adc_value - qcheck->adc_baseline);
					qcheck->rc_runtime.adc_trigger_start = qcheck->rc_runtime.adc_max * 4 / 5;
					qcheck->rc_runtime.adc_trigger_stop = qcheck->rc_runtime.adc_max / 5;
				}

				if (abs(adc_value - qcheck->adc_baseline) > qcheck->rc_runtime.adc_trigger_start) {
					qcheck->rc_runtime.udt_sum = 0;
				} else if (abs(adc_value - qcheck->adc_baseline) > qcheck->rc_runtime.adc_trigger_stop) {
					qcheck->rc_runtime.udt_sum += abs(adc_value - qcheck->adc_baseline) * dt;
				} else {
					qcheck->rc = qcheck->rc_runtime.udt_sum / (qcheck->rc_runtime.adc_trigger_start - qcheck->rc_runtime.adc_trigger_stop);
					qcheck->rc_runtime.phase = PRP_DONE;
					qcheck->rc_runtime.result = 1;
				}
			} else {
				qcheck->rc_runtime.phase = PRP_DONE;
			}

		}
		break;
	case PRP_DONE:
		break;
	}
	return qcheck->rc_runtime.phase == PRP_DONE ? 1 : 0;

#endif
}





void pzt_q_set_trigger_dF(PZTQ_CheckInfo * qcheck, int16_t df)
{
	qcheck->trigger_dF = df;
}

void pzt_q_check_test(void)
{
//	char buff[60];
//	pzt_q_check_baseline(&pztq_check2);
//	snprintf(buff, sizeof(buff), "baseline = %d+-%d\n", pztq_check2.adc_baseline, pztq_check2.adc_threshold);
//	interface_send(buff, strlen(buff));
//	pzt_q_check_rc(&pztq_check2);
//	snprintf(buff, sizeof(buff), "rc = %d\n", pztq_check2.rc);
//	interface_send(buff, strlen(buff));
}

