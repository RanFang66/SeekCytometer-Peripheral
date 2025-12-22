/*
 * modbus_master.h
 *
 * Created on: 2025年11月26日
 * Author: ranfa
 */

#ifndef INC_MODBUS_MASTER_H_
#define INC_MODBUS_MASTER_H_

#include "modbus_gateway.h"
#include "cmsis_os2.h"
#include "usart.h" // For HAL_UART_HandleTypeDef

#define MB_RX_BUFFER_SIZE	256

// --- Master Context Structure ---
typedef struct {
	target_t		target;
	uint8_t			addr;
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
void MB_Master_Init();
void MB_Master_StartTasks(void);

MB_Status_t MB_ReadHoldingRegs(target_t target, uint16_t startAddr, uint16_t quantity, uint16_t *destArray);
MB_Status_t MB_WriteSingleReg(target_t target, uint16_t regAddr, uint16_t value);
MB_Status_t MB_WriteMultipleRegs(target_t target, uint16_t startAddr, uint16_t quantity, uint16_t *sourceArray);


/* For called in HAL_UARTEx_RxEventCallback() */
void MB_UART_HandleRxEvent(UART_HandleTypeDef *huart, uint16_t size);

#endif /* INC_MODBUS_MASTER_H_ */
