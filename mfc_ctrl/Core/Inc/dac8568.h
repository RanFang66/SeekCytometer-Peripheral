/*
 * dac8568.h
 *
 *  Created on: Oct 22, 2025
 *      Author: ranfa
 */

#ifndef INC_DAC8568_H_
#define INC_DAC8568_H_

#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// DAC8568 command definition
#define DAC8568_WRITE_CHANNEL        	0x00
#define DAC8568_UPDATE_CHANNEL       	0x01
#define DAC8568_WRITE_UPDATE_ALL     	0x02
#define DAC8568_WRITE_UPDATE_CHANNEL 	0x03
#define DAC8568_POWER_DOWN          	0x04
#define DAC8568_CLEAR_CODE           	0x05
#define DAC8568_LDAC_SETUP           	0x06
#define DAC8568_SOFTWARE_RESET       	0x07



// DAC8568 channels definition
typedef enum {
    DAC8568_CH_A = 0x0,
    DAC8568_CH_B = 0x1,
    DAC8568_CH_C = 0x2,
    DAC8568_CH_D = 0x3,
    DAC8568_CH_E = 0x4,
    DAC8568_CH_F = 0x5,
    DAC8568_CH_G = 0x6,
    DAC8568_CH_H = 0x7,
    DAC8568_CH_ALL = 0x0F
} dac8568_channel_t;

typedef struct {
    SPI_HandleTypeDef *hspi;     // SPI handle
    GPIO_TypeDef      *sync_port;// GPIOB
    uint16_t           sync_pin;  // GPIO_PIN_12
    osMutexId_t        lock;      // Mutex for thread safety
    float              vref;      // Voltage reference
    bool               use_internal_ref; // If use internal voltage reference
} dac8568_t;

// Initialization of DAC8568
HAL_StatusTypeDef DAC8568_Init(dac8568_t *dev,
                               SPI_HandleTypeDef *hspi,
                               GPIO_TypeDef *sync_port, uint16_t sync_pin,
                               bool enable_internal_ref, float vref_volts);

// Write value to specific channel and update instantly
HAL_StatusTypeDef DAC8568_WriteUpdate(dac8568_t *dev,
                                      dac8568_channel_t ch,
                                      uint16_t code);

// Write value to specific channel but donot update
HAL_StatusTypeDef DAC8568_WriteInputOnly(dac8568_t *dev,
                                         dac8568_channel_t ch,
                                         uint16_t code);

// Broadcast: Write value to all channels and update instantly
HAL_StatusTypeDef DAC8568_BroadcastWriteUpdate(dac8568_t *dev,
                                               uint16_t code);

// Broadcast: Update all channels' value
HAL_StatusTypeDef DAC8568_UpdateAll(dac8568_t *dev);



#ifdef __cplusplus
}
#endif


#endif /* INC_DAC8568_H_ */
