/*
 * mcp4728.h
 *
 *  Created on: 2025年11月27日
 *      Author: ranfa
 */

#ifndef INC_MCP4728_H_
#define INC_MCP4728_H_


#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"

#ifdef __cplusplus
extern "C" {
#endif

// MCP4728 default address (address=0x60 when A0=A1=A2=GND )
#define MCP4728_DEFAULT_ADDRESS     0x60

// MCP4728 command definition
#define MCP4728_CMD_WRITEDAC        0x40
#define MCP4728_CMD_WRITEDACEEPROM  0x60

// DAC Channel
typedef enum {
    MCP4728_CHANNEL_A = 0,
    MCP4728_CHANNEL_B,
    MCP4728_CHANNEL_C,
    MCP4728_CHANNEL_D,
    MCP4728_CHANNEL_ALL
} MCP4728_Channel_t;

// DAC Values
typedef struct {
    uint16_t valueA;
    uint16_t valueB;
    uint16_t valueC;
    uint16_t valueD;
} MCP4728_Values_t;

// Message queue
typedef struct {
    MCP4728_Channel_t channel;
    uint16_t value;
} MCP4728_Message_t;

// Error code
typedef enum {
    MCP4728_OK = 0,
    MCP4728_ERROR,
    MCP4728_TIMEOUT
} MCP4728_Status_t;


// MCP4728 Type Define
typedef struct {
	GPIO_TypeDef *SCL_Port;
	GPIO_TypeDef *SDA_Port;
	GPIO_TypeDef *LDAC_Port;

	uint16_t SCL_Pin;
	uint16_t SDA_Pin;
	uint16_t LDAC_Pin;
} MCP4728_TypeDef;

/**
  * @brief Initialize MCP4728 chip
  * @note Need to be called before FreeRTOS start
  */
void MCP4728_Init(void);

/**
  * @brief Create MCP4728 control thread
  * @return thread handle id
  */
osThreadId_t MCP4728_Thread_Create(void);

/**
  * @brief Set DAC output
  * @param channel: DAC Channel
  * @param value: 12bit DAC value (0-4095)
  */
MCP4728_Status_t MCP4728_Set_Value(MCP4728_Channel_t channel, uint16_t value);

/**
  * @brief Set DAC output voltage
  * @param channel: DAC channel
  * @param voltage: output voltage (0-VREF)
  * @param vref: reference voltage
  */
MCP4728_Status_t MCP4728_Set_Voltage(MCP4728_Channel_t channel, float voltage, float vref);

#ifdef __cplusplus
}
#endif

#endif /* INC_MCP4728_H_ */
