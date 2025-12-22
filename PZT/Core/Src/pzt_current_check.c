#include "pzt_current_check.h"
#include "time.h"
#include "string.h"
static uint16_t pzt_check_adc_buffer[2048][2];
static uint32_t d_tick, tick;
//static uint16_t adc_active_threshold = 1700;
//static uint32_t adc_active_pick_sum;
//static uint16_t adc_active_pick_count;
//static uint16_t adc_active_pick_ave = 0;
//static uint16_t adc_active_pick = 0;
static uint32_t adc_active_pick_timestamp;
//static uint16_t adc_active = 0;

typedef struct {
	uint16_t threshold;
	uint16_t peak_ave;
	uint32_t peak_sum;
	uint16_t peak_count;
	uint16_t peak;
	uint16_t active;
	uint16_t stop_threshold;
	uint16_t stop_trigger_cnt;
} PCC_Vpp_Info;

static PCC_Vpp_Info pcc_ch1 = {
	.threshold = 1200,
	.stop_threshold = 4095
};
static PCC_Vpp_Info pcc_ch2 = {
	.threshold = 1200,
	.stop_threshold = 4095
};
typedef enum {
	PCS_IDLE = 0,
	PCS_RUN
} PZT_Check_State;
static PZT_Check_State pzt_check_state = PCS_IDLE;
// 采样周期 27clk/21M x 1024 = 1316 us
// 经测试 计算过程用时 约300us
// 计算过程要尽量快
void pzt_adc_dma_halfcplt_callback(void)
{
	tick = SysTick->VAL;
	uint16_t *p = pzt_check_adc_buffer[0];
	while (p < pzt_check_adc_buffer[1024]) {
		if (!pcc_ch1.active && p[0] > pcc_ch1.threshold) {
			pcc_ch1.active = 1;
		} else if (pcc_ch1.active && p[0] < pcc_ch1.threshold) {
			pcc_ch1.active = 0;
			pcc_ch1.peak_count ++;
			pcc_ch1.peak_sum += pcc_ch1.peak;
			pcc_ch1.peak = 0;
		}

		if (!pcc_ch2.active && p[1] > pcc_ch2.threshold) {
			pcc_ch2.active = 1;
		} else if (pcc_ch2.active && p[1] < pcc_ch2.threshold) {
			pcc_ch2.active = 0;
			pcc_ch2.peak_count ++;
			pcc_ch2.peak_sum += pcc_ch2.peak;
			pcc_ch2.peak = 0;
		}
//		if (!adc_active && *p > adc_active_threshold) {
//			adc_active = 1;
//			adc_active_pick_count ++;
//		} else if (adc_active && *p < adc_active_threshold) {
//			adc_active = 0;
//			adc_active_pick_sum += adc_active_pick;
//			adc_active_pick = 0;
//		}

		if (pcc_ch1.active) {
			if (pcc_ch1.peak < p[0]) {
				pcc_ch1.peak = p[0];
			}
		}
		if (pcc_ch2.active) {
			if (pcc_ch2.peak < p[1]) {
				pcc_ch2.peak = p[1];
			}
		}
		p += 2;
	}
	uint32_t t = SysTick->VAL;
	if (tick > t) {
		d_tick = tick - t;
	}
}

void pzt_adc_dma_cplt_callback(void)
{
	tick = SysTick->VAL;
	uint16_t *p = pzt_check_adc_buffer[1024];
	while (p < pzt_check_adc_buffer[2048]) {
		if (!pcc_ch1.active && p[0] > pcc_ch1.threshold) {
			pcc_ch1.active = 1;
		} else if (pcc_ch1.active && p[0] < pcc_ch1.threshold) {
			pcc_ch1.active = 0;
			pcc_ch1.peak_count ++;
			pcc_ch1.peak_sum += pcc_ch1.peak;
			pcc_ch1.peak = 0;
		}

		if (!pcc_ch2.active && p[1] > pcc_ch2.threshold) {
			pcc_ch2.active = 1;
		} else if (pcc_ch2.active && p[1] < pcc_ch2.threshold) {
			pcc_ch2.active = 0;
			pcc_ch2.peak_count ++;
			pcc_ch2.peak_sum += pcc_ch2.peak;
			pcc_ch2.peak = 0;
		}
//		if (!adc_active && *p > adc_active_threshold) {
//			adc_active = 1;
//			adc_active_pick_count ++;
//		} else if (adc_active && *p < adc_active_threshold) {
//			adc_active = 0;
//			adc_active_pick_sum += adc_active_pick;
//			adc_active_pick = 0;
//		}

		if (pcc_ch1.active) {
			if (pcc_ch1.peak < p[0]) {
				pcc_ch1.peak = p[0];
			}
		}
		if (pcc_ch2.active) {
			if (pcc_ch2.peak < p[1]) {
				pcc_ch2.peak = p[1];
			}
		}
		p += 2;
	}
	uint32_t t = SysTick->VAL;
	if (tick > t) {
		d_tick = tick - t;
	}
}


void pzt_check_start(void)
{
	if (pzt_check_state == PCS_RUN)
		return;
	HAL_ADC_Start_DMA(&hadc1, (uint32_t *)pzt_check_adc_buffer, sizeof(pzt_check_adc_buffer) / sizeof(uint16_t));
	pcc_ch1.peak_sum = pcc_ch1.peak_count = 0;
	pcc_ch2.peak_sum = pcc_ch2.peak_count = 0;
	pzt_check_state = PCS_RUN;
	bsp_time_clear(adc_active_pick_timestamp);
}

void pzt_check_stop(void)
{

	HAL_ADC_Stop_DMA(&hadc1);
	pzt_check_state = PCS_IDLE;
	pcc_ch1.stop_trigger_cnt = 0;
	pcc_ch2.stop_trigger_cnt = 0;
}

uint16_t pzt_get_peak(uint16_t ch)
{
	if (ch == 1) {
		return pcc_ch1.peak_ave;
	} else if (ch == 2) {
		return pcc_ch2.peak_ave;
	} else
		return 0;
}

uint16_t pzt_is_run(void)
{
	return pzt_check_state == PCS_RUN;
}
void pzt_set_target(uint16_t ch, uint16_t target)
{
	if (ch == 1) {
		pcc_ch1.stop_threshold = target;
	} else if (ch == 2) {
		pcc_ch2.stop_threshold = target;
	}
}

uint16_t pzt_get_press_ok(uint16_t ch)
{
	if (ch == 1) {
		return pcc_ch1.stop_trigger_cnt >= 3;
	} else if (ch == 2) {
		return pcc_ch2.stop_trigger_cnt >= 3;
	} else
		return 0;
}




void pzt_check(void)
{
	if (pzt_check_state == PCS_RUN) {
		if (bsp_time_more(adc_active_pick_timestamp, 100)) {
			__disable_irq();
			if (pcc_ch1.peak_count) {
				pcc_ch1.peak_ave = pcc_ch1.peak_sum / pcc_ch1.peak_count;
				pcc_ch1.peak_sum = pcc_ch1.peak_count = 0;
				if (pcc_ch1.stop_trigger_cnt < 3) {
					if (pcc_ch1.peak_ave > pcc_ch1.stop_threshold)
						pcc_ch1.stop_trigger_cnt ++;
					else
						pcc_ch1.stop_trigger_cnt = 0;
				}

			}

			if (pcc_ch2.peak_count) {
				pcc_ch2.peak_ave = pcc_ch2.peak_sum / pcc_ch2.peak_count;
				pcc_ch2.peak_sum = pcc_ch2.peak_count = 0;
				if (pcc_ch2.stop_trigger_cnt < 3) {
					if (pcc_ch2.peak_ave > pcc_ch2.stop_threshold)
						pcc_ch2.stop_trigger_cnt ++;
					else
						pcc_ch2.stop_trigger_cnt = 0;
				}
			}
			uint8_t send_buffer[40];
			sprintf(send_buffer, "peak %d\t%d", pcc_ch1.peak_ave, pcc_ch2.peak_ave);
			interface_send(send_buffer, strlen(send_buffer));
//			if (adc_active_pick_count) {
//				adc_active_pick_ave = adc_active_pick_sum / adc_active_pick_count;
//				adc_active_pick_sum = 0;
//				adc_active_pick_count = 0;
//			}
			__enable_irq();
			bsp_time_clear(adc_active_pick_timestamp);
		}
	}

}
