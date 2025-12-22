/*
 * modbus_master.h
 *
 *  Created on: 2025年11月28日
 *      Author: ranfa
 */

#ifndef INC_MODBUS_MASTER_H_
#define INC_MODBUS_MASTER_H_



#include "modbus_defs.h"
#include "usart.h"
#include "cmsis_os2.h"



typedef enum {
	MB_OK = 0,
	MB_ERROR_TIMEOUT,
	MB_ERROR_CRC,
	MB_ERROR_SLAVE_EXCEP,
	MB_ERROR_WRONG_RESPONSE,
	MB_ERROR_BUSY,
	MB_ERROR_INVALID_PARA,
	MB_ERROR_NOMEM,
	MB_ERROR_HAL,
} MB_Status_t;

// --- Master Context Structure ---
typedef struct {
	UART_HandleTypeDef *huart;

	uint8_t 		rxDMABuffer[MB_RX_BUFFER_SIZE];
	uint8_t 		txBuffer[MB_MAX_FRAME_SIZE];
	uint8_t 		rxFrameBuffer[MB_MAX_FRAME_SIZE];

	osMutexId_t 	txMutex;
	osThreadId_t 	taskHandle;
	osThreadId_t 	callingTaskHandle; // Thread waiting for response

	uint16_t 		rxFrameLen;
} MB_MasterCtx_t;


// --- Public API ---
MB_Status_t MB_Master_Init();
void MB_Master_StartTask(void);

MB_Status_t MB_ReadHoldingRegs(uint8_t slaveId, uint16_t startAddr, uint16_t quantity, uint16_t *destArray);
MB_Status_t MB_WriteSingleReg(uint8_t slaveId, uint16_t regAddr, uint16_t value);
MB_Status_t MB_WriteMultipleRegs(uint8_t slaveId, uint16_t startAddr, uint16_t quantity, uint16_t *sourceArray);

/* 32 Bits Data operation */
MB_Status_t MB_Read32BitsWord(uint8_t slaveId, uint16_t startAddr,  uint32_t *data);
MB_Status_t MB_Write32BitsWord(uint8_t slaveId, uint16_t startAddr, uint32_t val);

/* For called in HAL_UARTEx_RxEventCallback() */
void MB_UART_HandleRxEvent(UART_HandleTypeDef *huart, uint16_t size);



#endif /* INC_MODBUS_MASTER_H_ */
