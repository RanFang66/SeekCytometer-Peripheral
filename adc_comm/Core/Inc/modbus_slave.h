/*
 * modbus_slave.h
 *
 *  Created on: 2025年11月26日
 *      Author: ranfa
 */

#ifndef INC_MODBUS_SLAVE_H_
#define INC_MODBUS_SLAVE_H_


#include "modbus_gateway.h"
#include "modbus_master.h" // Needed for master API usage
#include "cmsis_os2.h"
#include "usart.h" // For HAL_UART_HandleTypeDef

#define MB_SLAVE_RX_BUFFER_SIZE		MB_MAX_FRAME_SIZE
#define MB_SLAVE_QUEUE_DEPTH		8

// Structure for passing received frame data from ISR/Callback to Slave Task
typedef struct {
	uint16_t length;
	uint8_t frame[MB_MAX_FRAME_SIZE];
} MB_FrameMsg_t;


typedef struct {
	UART_HandleTypeDef *huart;
	uint8_t addr; 		// MB_LOCAL_ADDR (0x01)

	// Buffers
	uint8_t rxDMABuffer[MB_SLAVE_RX_BUFFER_SIZE];
	uint8_t txBuffer[MB_MAX_FRAME_SIZE];

	// Local Register storage
	uint16_t* local_registers[MB_REG_LOCAL_COUNT];

	// RTOS Handles
	osThreadId_t taskHandle;
	osMessageQueueId_t rxQueue;

	// Master context access (to be implemented in modbus_master.c)
	MB_MasterCtx_t *mbMotor;
	MB_MasterCtx_t *mbMFC;
	MB_MasterCtx_t *mbPZT;

} MB_SlaveCtx_t;


void MB_Slave_Init();
void MB_Slave_StartTask(void);

void MB_Slave_DefineReg(uint16_t idx, uint16_t *valP);


// Must be called from HAL_UARTEx_RxEventCallback() for the slave UART (huart6)
void MB_SLAVE_UART_HandleRxEvent(UART_HandleTypeDef *huart, uint16_t size);


#endif /* INC_MODBUS_SLAVE_H_ */
