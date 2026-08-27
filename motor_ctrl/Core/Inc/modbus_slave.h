/*
 * modbus_slave.h
 *
 *  Created on: 2025年12月4日
 *      Author: ranfa
 */

#ifndef INC_MODBUS_SLAVE_H_
#define INC_MODBUS_SLAVE_H_

#include "modbus_defs.h"
#include "cmsis_os2.h"
#include "usart.h" // For HAL_UART_HandleTypeDef

#define MB_SLAVE_RX_BUFFER_SIZE		MB_MAX_FRAME_SIZE
#define MB_SLAVE_QUEUE_DEPTH		8
#define MB_LOCAL_ADDR				0x22U
// Structure for passing received frame data from ISR/Callback to Slave Task
typedef struct {
	uint16_t length;
	uint8_t frame[MB_MAX_FRAME_SIZE];
} MB_FrameMsg_t;


typedef struct {
	UART_HandleTypeDef *huart;
	uint8_t addr; 		// MB_LOCAL_ADDR (0x22)

	// Buffers
	uint8_t rxDMABuffer[MB_SLAVE_RX_BUFFER_SIZE];
	uint8_t txBuffer[MB_MAX_FRAME_SIZE];

	// Local Register storage
	uint16_t* local_registers[MB_REG_LOCAL_COUNT];

	// RTOS Handles
	osThreadId_t taskHandle;
	osMessageQueueId_t rxQueue;
} MB_SlaveCtx_t;


/* Slave link health, filled by MB_Slave_GetDiag() for the "mb -i" shell command */
typedef struct {
	uint32_t rxEvtCount;	/* UART RX-event callbacks */
	uint32_t processCount;	/* frames handed to the task */
	uint32_t validCount;	/* frames that passed addr/CRC/range checks */
	uint32_t errCount;	/* UART error callbacks */
	uint32_t errCode;	/* OR of HAL_UART_ERROR_* : PE 0x01 NE 0x02 FE 0x04 ORE 0x08 */
	uint32_t rearmFail;	/* RX re-arm refused inside the error ISR */
	uint32_t wdRearm;	/* RX re-armed by the task watchdog */
	uint8_t  rxState;	/* huart->RxState, 0x22 = HAL_UART_STATE_BUSY_RX */
} MB_SlaveDiag_t;

void MB_Slave_Init();
void MB_Slave_StartTask(void);

void MB_Slave_DefineReg(uint16_t idx, uint16_t *valP);


// Must be called from HAL_UARTEx_RxEventCallback() for the slave UART (huart6)
void MB_SLAVE_UART_HandleRxEvent(UART_HandleTypeDef *huart, uint16_t size);

// Must be called from HAL_UART_TxCpltCallback() for the slave UART
void MB_SLAVE_UART_HandleTxCplt(UART_HandleTypeDef *huart);

// Must be called from HAL_UART_ErrorCallback() for the slave UART. Re-arms the
// RX DMA that the HAL aborts on a line error - without it one glitch is fatal.
void MB_SLAVE_UART_HandleError(UART_HandleTypeDef *huart);

void MB_Slave_GetDiag(MB_SlaveDiag_t *diag);
void MB_Slave_ClearDiag(void);




#endif /* INC_MODBUS_SLAVE_H_ */
