/*
 * hr4988.h
 *
 *  Created on: 2025年11月27日
 *      Author: ranfa
 */

#ifndef INC_HR4988_H_
#define INC_HR4988_H_

#include "tim.h"
#include "gpio.h"
#include "cmsis_os2.h"



typedef enum {
	HR4988_DISABLED = 0,
	HR4988_IDLE,
	HR4988_RUN_CW,
	HR4988_RUN_CCW,
	HR4988_FAULT,
} HR4988_Status_t;

typedef enum {
	HR4988_DIR_CW = 0,
	HR4988_DIR_CCW,
} HR4988_Dir_t;

#define HR4988_DEFAULT_FREQ (320)

typedef struct {
	GPIO_TypeDef* 		gpioEn;
	uint16_t 			pinEn;
	GPIO_TypeDef* 		gpioDir;
	uint16_t 			pinDir;
	TIM_HandleTypeDef 	*htim;
	uint32_t			pwmChannel;
	HR4988_Status_t		status;
	HR4988_Dir_t 		dir;
	uint16_t			freq;
	osMutexId_t			lock;
} HR4988_TypeDef;


void HR4988_Init(HR4988_TypeDef *hr4988,
		GPIO_TypeDef *gpioEn, uint16_t pinEn, GPIO_TypeDef *gpioDir, uint16_t pinDir,
		TIM_HandleTypeDef *htim, uint32_t pwmCh);

void HR4988_Enable(HR4988_TypeDef *hr4988);
void HR4988_Disable(HR4988_TypeDef *hr4988);
void HR4988_SetDirection(HR4988_TypeDef *hr4988, uint8_t dir);
void HR4988_SetFreq(HR4988_TypeDef *hr4988, uint32_t freq);
void HR4988_RunCW(HR4988_TypeDef *hr4988, uint16_t freq);
void HR4988_RunCCW(HR4988_TypeDef *hr4988, uint16_t freq);


#endif /* INC_HR4988_H_ */
