/*
 * modbus_master.c
 *
 * Created on: 2025年11月26日
 * Author: ranfa
 */

#include "modbus_master.h"
#include <string.h>
#include "bsp_uart.h"


// --- Master Context Structures (Not Pointers!) ---
static MB_MasterCtx_t mbMotor;
static MB_MasterCtx_t mbMFC;
static MB_MasterCtx_t mbPZT;


/**
 * @brief Utility to get the context pointer for a given target
 */
MB_MasterCtx_t *getMBMasterCtx(target_t target) {
	switch (target) {
	case TARGET_MOTOR:
		return &mbMotor;
	case TARGET_MFC:
		return &mbMFC;
	case TARGET_PZT:
		return &mbPZT;
	default:
		return NULL;
	}
}

static MB_Status_t MB_MasterInitHelper(target_t target, const char *name, UART_HandleTypeDef *huart, uint8_t addr)
{
	if (huart == NULL) {
		return MB_ERROR_INVALID_PARA;
	}
	MB_MasterCtx_t *ctx = getMBMasterCtx(target);
	if (ctx == NULL) {
		return MB_ERROR_INVALID_PARA;
	}

	ctx->target = target;
	ctx->addr = addr;
	ctx->huart = huart;
	ctx->rxFrameLen = 0;

	osMutexAttr_t mutexAttr = {.name = name};
	ctx->txMutex = osMutexNew(&mutexAttr);
	if (ctx->txMutex == NULL) {
		return MB_ERROR_NOMEM;
	}

	return MB_OK;
}

/**
 * @brief Initializes all three Modbus masters
 */
void MB_Master_Init()
{
	// Corrected: Uses &MOTOR_UART_HANDLE, &MFC_UART_HANDLE, &PZT_UART_HANDLE pointers
	MB_MasterInitHelper(TARGET_MOTOR, "motorMB", &MB_MOTOR_UART_HANDLE, MB_MOTOR_ADDR);
	MB_MasterInitHelper(TARGET_MFC, "mfcMB", &MB_MFC_UART_HANDLE, MB_MFC_ADDR);
    // CRITICAL FIX: Correctly initializes TARGET_PZT
	MB_MasterInitHelper(TARGET_PZT, "pztMB", &MB_PZT_UART_HANDLE, MB_PZT_ADDR);
}


/**
 * @brief Sends the request frame, including the calculated CRC.
 * @param ctx The master context.
 * @param len The length of the Modbus PDU (excluding ID, CRC).
 * @return HAL_StatusTypeDef
 */
static HAL_StatusTypeDef SendRequest(MB_MasterCtx_t *ctx, uint16_t len)
{
	if (ctx == NULL) {
		return HAL_ERROR;
	}

    uint16_t crc = MB_CalculateCRC16(ctx->txBuffer, len);
    // FIX: Use ctx->txBuffer, not non-existent global 'mbm.txBuffer'
    ctx->txBuffer[len] = crc & 0xFF;         // CRC Low
    ctx->txBuffer[len + 1] = (crc >> 8) & 0xFF; // CRC High

    // Blocking TX (Master typically uses blocking TX)
    return HAL_UART_Transmit(ctx->huart, ctx->txBuffer, len + 2, MB_DEFAULT_TIMEOUT_MS);

}

/**
 * @brief Executes the transaction: sends request, waits for response, validates response.
 * @param ctx The master context.
 * @param txPDUlen The length of the Modbus Request PDU (e.g., 6 for FC03/06).
 * @param expectedFC The function code expected in the response.
 * @param expectedDataLen The expected length of the data field (or PDU len for echo responses).
 */
static MB_Status_t ExecuteTransaction(MB_MasterCtx_t *ctx, uint16_t txPDUlen, uint8_t expectedFC, uint16_t expectedDataLen)
{
	if (ctx == NULL) {
		return MB_ERROR_INVALID_PARA;
	}

    MB_Status_t status = MB_OK;

    // 0. Enable UART DMA Interrupt
	HAL_StatusTypeDef st = HAL_UARTEx_ReceiveToIdle_DMA(ctx->huart, ctx->rxDMABuffer, MB_RX_BUFFER_SIZE);
	if (st == HAL_OK) {
		// Disable Half-Transfer Interrupt (we only care about Idle)
		__HAL_DMA_DISABLE_IT(ctx->huart->hdmarx, DMA_IT_HT);
	} else {
		return MB_ERROR_HAL;
	}


    // 1. Send the Request
    if (SendRequest(ctx, txPDUlen) != HAL_OK) {
        return MB_ERROR_TIMEOUT; // Or specific HAL error if desired
    }

    // 2. Wait for response signal (set by MB_UART_HandleRxEvent)
    ctx->callingTaskHandle = osThreadGetId();
    uint32_t flags = osThreadFlagsWait(0x01, osFlagsWaitAny, MB_DEFAULT_TIMEOUT_MS);
    ctx->callingTaskHandle = NULL; // Clear handle

    if (!(flags & 0x01)) {
    	HAL_UART_AbortReceive_IT(ctx->huart);
        return MB_ERROR_TIMEOUT;
    }

    // --- Response Validation ---

    // Check minimum length (ID, FC, Data/ExcepCode, CRC_L, CRC_H)
    if (ctx->rxFrameLen < 5) return MB_ERROR_WRONG_RESPONSE;

    // Check CRC
    uint16_t receivedCRC = (ctx->rxFrameBuffer[ctx->rxFrameLen - 1] << 8) | ctx->rxFrameBuffer[ctx->rxFrameLen - 2];
    uint16_t calculatedCRC = MB_CalculateCRC16(ctx->rxFrameBuffer, ctx->rxFrameLen - 2);
    if (receivedCRC != calculatedCRC) return MB_ERROR_CRC;

    // Check Slave ID
    if (ctx->rxFrameBuffer[0] != ctx->addr) return MB_ERROR_WRONG_RESPONSE;

    // Check Function Code (Exception or Normal)
    uint8_t receivedFC = ctx->rxFrameBuffer[1];

    if (receivedFC == (expectedFC | 0x80)) {
        // Exception response: [ID][FC|0x80][ExceptionCode][CRC]
        return MB_ERROR_SLAVE_EXCEP;
    } else if (receivedFC != expectedFC) {
        return MB_ERROR_WRONG_RESPONSE;
    }

    // Check Expected Data Length (Specific to FC 0x03)
    if (expectedFC == MB_FC_READ_HOLDING_REGS) {
        // For FC 0x03, byte 2 is the Byte Count.
        if (ctx->rxFrameBuffer[2] != expectedDataLen) {
        	return MB_ERROR_WRONG_RESPONSE;
        }
    }
    // For FC 0x06 and 0x10 response (PDU length check)
    else if (ctx->rxFrameLen - 2 != expectedDataLen) {
        return MB_ERROR_WRONG_RESPONSE;
    }

    return status;
}


/**
 * @brief Reads contiguous block of holding registers (FC 0x03)
 */
MB_Status_t MB_ReadHoldingRegs(target_t target, uint16_t startAddr, uint16_t quantity, uint16_t *destArray)
{
	MB_MasterCtx_t *ctx = getMBMasterCtx(target);
	if (ctx == NULL) return MB_ERROR_INVALID_PARA;

	if (osMutexAcquire(ctx->txMutex, osWaitForever) != osOK) return MB_ERROR_BUSY;

	MB_Status_t status = MB_ERROR_WRONG_RESPONSE;
    uint16_t txPDUlen = 6; // PDU length for FC03 request

	// 1. Build Request Frame
	ctx->txBuffer[0] = ctx->addr;
	ctx->txBuffer[1] = MB_FC_READ_HOLDING_REGS;
	ctx->txBuffer[2] = (startAddr >> 8) & 0xFF;
	ctx->txBuffer[3] = startAddr & 0xFF;
	ctx->txBuffer[4] = (quantity >> 8) & 0xFF;
	ctx->txBuffer[5] = quantity & 0xFF;

	// Expected response data length (2 bytes per register)
	uint16_t expectedByteCount = quantity * 2;

    // 2. Execute Transaction
	status = ExecuteTransaction(ctx, txPDUlen, MB_FC_READ_HOLDING_REGS, expectedByteCount);

	if (status == MB_OK) {
		// 3. Parse Data (starts at offset 3: [ID][FC][Count]...)
		uint8_t *pData = &(ctx->rxFrameBuffer[3]);
		for (int i = 0; i < quantity; i++) {
			// Modbus is Big Endian (MSB first)
			destArray[i] = (pData[i*2] << 8) | pData[i*2 + 1];
		}
	}

	osMutexRelease(ctx->txMutex);
	return status;
}

/**
 * @brief Writes a single holding register (FC 0x06)
 */
MB_Status_t MB_WriteSingleReg(target_t target, uint16_t regAddr, uint16_t value)
{
	MB_MasterCtx_t *ctx = getMBMasterCtx(target);
	if (ctx == NULL) return MB_ERROR_INVALID_PARA;

	if (osMutexAcquire(ctx->txMutex, osWaitForever) != osOK) return MB_ERROR_BUSY;

    uint16_t txPDUlen = 6; // PDU length for FC06 request
    uint16_t responsePDUlen = 6; // PDU length for FC06 response (echo)

	// 1. Build Request Frame
	ctx->txBuffer[0] = ctx->addr;
	ctx->txBuffer[1] = MB_FC_WRITE_SINGLE_REG;
	ctx->txBuffer[2] = (regAddr >> 8) & 0xFF;
	ctx->txBuffer[3] = regAddr & 0xFF;
	ctx->txBuffer[4] = (value >> 8) & 0xFF;   // Value MSB
	ctx->txBuffer[5] = value & 0xFF;          // Value LSB

	// 2. Execute Transaction
	MB_Status_t status = ExecuteTransaction(ctx, txPDUlen, MB_FC_WRITE_SINGLE_REG, responsePDUlen);

	osMutexRelease(ctx->txMutex);
	return status;
}

/**
 * @brief Writes multiple holding registers (FC 0x10)
 */
MB_Status_t MB_WriteMultipleRegs(target_t target, uint16_t startAddr, uint16_t quantity, uint16_t *sourceArray)
{
	MB_MasterCtx_t *ctx = getMBMasterCtx(target);
	if (ctx == NULL) return MB_ERROR_INVALID_PARA;

	if (osMutexAcquire(ctx->txMutex, osWaitForever) != osOK) return MB_ERROR_BUSY;

	// 1. Build Request Frame
	uint8_t byteCount = quantity * 2;
	uint16_t txPDUlen = 7 + byteCount; // 7 bytes header + data
    uint16_t responsePDUlen = 6; // PDU length for FC10 response

	ctx->txBuffer[0] = ctx->addr;
	ctx->txBuffer[1] = MB_FC_WRITE_MULTI_REGS;
	ctx->txBuffer[2] = (startAddr >> 8) & 0xFF;
	ctx->txBuffer[3] = startAddr & 0xFF;
	ctx->txBuffer[4] = (quantity >> 8) & 0xFF;
	ctx->txBuffer[5] = quantity & 0xFF;
	ctx->txBuffer[6] = byteCount;

	uint8_t *pData = &(ctx->txBuffer[7]);
	for (int i = 0; i < quantity; i++) {
		pData[i*2]     = (sourceArray[i] >> 8) & 0xFF; // MSB
		pData[i*2 + 1] = sourceArray[i] & 0xFF;      // LSB
	}

	// 2. Execute Transaction
	MB_Status_t status = ExecuteTransaction(ctx, txPDUlen, MB_FC_WRITE_MULTI_REGS, responsePDUlen);

	osMutexRelease(ctx->txMutex);
	return status;
}

/**
 * @brief Handles the RX Complete/Idle event for the UART.
 * This function should be called from the HAL_UARTEx_RxEventCallback in your main code.
 */
static void MB_UartRxCallback(MB_MasterCtx_t *ctx, uint16_t size)
{
	if (ctx == NULL) return;

	if (ctx->callingTaskHandle != NULL)
	{
		/* 1. Determine Frame Length */
		ctx->rxFrameLen = size;

		/* 2. Copy received data from DMA buffer to transaction buffer */
		memcpy(ctx->rxFrameBuffer, ctx->rxDMABuffer, ctx->rxFrameLen);

		/* 3. Signal the waiting application task */
		osThreadFlagsSet(ctx->callingTaskHandle, 0x01);
	}
}


/**
 * @brief Public interface for the HAL_UARTEx_RxEventCallback
 * This function must be called inside the HAL_UARTEx_RxEventCallback.
 */
void MB_UART_HandleRxEvent(UART_HandleTypeDef *huart, uint16_t size)
{
    // FIX: Use the dot operator to access huart member
	if (huart == mbMotor.huart) {
		MB_UartRxCallback(&mbMotor, size);
	} else if (huart == mbMFC.huart) {
		MB_UartRxCallback(&mbMFC, size);
	} else if (huart == mbPZT.huart) {
		MB_UartRxCallback(&mbPZT, size);
	}
}


/**
 * @brief Task for each Modbus Master (mostly idle)
 * @note The transaction logic is synchronous and handled in the API calls.
 */
static void MB_Master_Task(void *argument)
{
    while(1)
    {
        osDelay(100);
    }
}

/**
 * @brief Starts the FreeRTOS tasks for all three masters
 */
void MB_Master_StartTasks(void)
{
    const osThreadAttr_t mbMotorTask_attributes = {
        .name = "ModbusMotor",
        .stack_size = 256 * 4,
        .priority = (osPriority_t) osPriorityNormal,
    };
    // FIX: Use the dot operator to access taskHandle
    mbMotor.taskHandle = osThreadNew(MB_Master_Task, (void*)&mbMotor, &mbMotorTask_attributes);

    const osThreadAttr_t mbMFCTask_attributes = {
		.name = "ModbusMFC",
		.stack_size = 512 * 4,
		.priority = (osPriority_t) osPriorityNormal,
	};
	mbMFC.taskHandle = osThreadNew(MB_Master_Task, (void*)&mbMFC, &mbMFCTask_attributes);

	const osThreadAttr_t mbPZTTask_attributes = {
		.name = "ModbusPZT",
		.stack_size = 512 * 4,
		.priority = (osPriority_t) osPriorityNormal,
	};
	mbPZT.taskHandle = osThreadNew(MB_Master_Task, (void*)&mbPZT, &mbPZTTask_attributes);

}
