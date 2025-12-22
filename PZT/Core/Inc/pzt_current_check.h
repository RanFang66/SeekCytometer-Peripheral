#ifndef __PZT_CURRENT_CHECK_H
#define __PZT_CURRENT_CHECK_H
#include "main.h"
#include "adc.h"


void pzt_check_start(void);
void pzt_check_stop(void);
void pzt_adc_dma_halfcplt_callback(void);
void pzt_adc_dma_cplt_callback(void);

void pzt_check(void);

uint16_t pzt_get_peak(uint16_t ch);
uint16_t pzt_is_run(void);
void pzt_set_target(uint16_t ch, uint16_t target);
uint16_t pzt_get_press_ok(uint16_t ch);
#endif
