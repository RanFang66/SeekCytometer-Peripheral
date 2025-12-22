/*
 * sloenoid_pwm.c
 *
 *  Created on: Oct 24, 2025
 *      Author: ranfa
 */

#include "sol_valve_control.h"
#include <string.h>
#include <stdlib.h>
#include "debug_shell.h"
#include "bsp_gpio.h"



static GPIO_TypeDef *const gpioList[] = {SOLE_VALVE_1_GPIO, SOLE_VALVE_2_GPIO, SOLE_VALVE_3_GPIO, SOLE_VALVE_4_GPIO, SOLE_VALVE_5_GPIO};
static const uint16_t pinList[] = {SOLE_VALVE_1_PIN, SOLE_VALVE_2_PIN, SOLE_VALVE_3_PIN, SOLE_VALVE_4_PIN, SOLE_VALVE_5_PIN};

static sol_ch_t     soleValve[SOL_VALVE_COUNT];
static osMutexId_t  soleLock = NULL;



// ----------------- 初始化 -----------------
HAL_StatusTypeDef SOL_Init()
{
    for (int i = 0; i < SOL_VALVE_COUNT; ++i) {
    	soleValve[i].gpioEn = gpioList[i];
    	soleValve[i].pinEn = pinList[i];
    	soleValve[i].state = SOL_STATE_OFF;
		HAL_GPIO_WritePin(soleValve[i].gpioEn, soleValve[i].pinEn, GPIO_PIN_RESET);
    }

    // 互斥
    osMutexAttr_t mattr = { .name = "sole_lock" };
    soleLock = osMutexNew(&mattr);
    if (!soleLock) {
    	return HAL_ERROR;
    }

    return HAL_OK;
}

// ----------------- API -----------------
HAL_StatusTypeDef SOL_Open(uint8_t id)
{
    if (id >= SOL_VALVE_COUNT) {
    	return HAL_ERROR;
    }

    osMutexAcquire(soleLock, osWaitForever);

	HAL_GPIO_WritePin(soleValve[id].gpioEn, soleValve[id].pinEn, GPIO_PIN_SET);
    soleValve[id].state = SOL_STATE_ON;

    osMutexRelease(soleLock);
    return HAL_OK;
}

HAL_StatusTypeDef SOL_Close(uint8_t id)
{
    if (id >= SOL_VALVE_COUNT) {
    	return HAL_ERROR;
    }

    osMutexAcquire(soleLock, osWaitForever);

	HAL_GPIO_WritePin(soleValve[id].gpioEn, soleValve[id].pinEn, GPIO_PIN_RESET);
    soleValve[id].state = SOL_STATE_OFF;

    osMutexRelease(soleLock);
    return HAL_OK;
}

HAL_StatusTypeDef SOL_OpenAll()
{
    osMutexAcquire(soleLock, osWaitForever);

    for (uint8_t i = 0; i < SOL_VALVE_COUNT; i++) {
    	HAL_GPIO_WritePin(soleValve[i].gpioEn, soleValve[i].pinEn, GPIO_PIN_SET);
    	soleValve[i].state = SOL_STATE_ON;
    }

    osMutexRelease(soleLock);
    return HAL_OK;
}

HAL_StatusTypeDef SOL_CloseAll()
{
    osMutexAcquire(soleLock, osWaitForever);

    for (uint8_t i = 0; i < SOL_VALVE_COUNT; i++) {
    	HAL_GPIO_WritePin(soleValve[i].gpioEn, soleValve[i].pinEn, GPIO_PIN_RESET);
    	soleValve[i].state = SOL_STATE_OFF;
    }

    osMutexRelease(soleLock);
    return HAL_OK;
}


sol_state_t SOL_GetState(uint8_t id)
{
    if (id >= SOL_VALVE_COUNT) {
    	return SOL_STATE_OFF;
    }
    return soleValve[id].state;
}



