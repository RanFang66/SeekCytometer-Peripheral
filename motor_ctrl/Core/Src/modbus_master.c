/*
 * modbus_master.c
 *
 *  Created on: 2025年11月28日
 *      Author: ranfa
 */


#include "modbus_master.h"
#include "bsp_uart.h"
#include "debug_shell.h"


static MB_MasterCtx_t mbMaster;

/* Master link error counters, reported by the "mb -i" shell command. */
static volatile uint32_t mbMasterErrCount = 0;
static volatile uint32_t mbMasterErrCode  = 0;


/* Thread flags signalled to the task waiting inside ExecuteTransaction. */
#define MB_MASTER_RESP_FLAG     0x01U   /* response frame received (RX IDLE)  */
#define MB_MASTER_TXCPLT_FLAG   0x02U   /* request frame fully transmitted    */


MB_Status_t MB_Master_Init()
{
	mbMaster.huart = &STEPPER_MOTOR_UART_HANDLE;

	mbMaster.rxFrameLen = 0;
	osMutexAttr_t mutexAttr = {.name = "MB_Master"};
	mbMaster.txMutex = osMutexNew(&mutexAttr);
	if (mbMaster.txMutex == NULL) {
		LOG_ERROR("Modbus master create mutex failed!");
		return MB_ERROR_NOMEM;
	}
	// Start DMA reception immediately

	return MB_OK;
}

static uint16_t MB_CalculateCRC16(const uint8_t *buffer, uint16_t length)
{
	uint16_t crc = 0xFFFF;
	for (uint16_t i = 0; i < length; i++) {
		crc ^= buffer[i];
		for (int j = 0; j < 8; j++) {
			if (crc & 1) crc = (crc >> 1) ^ 0xA001;
			else crc >>= 1;
		}
	}
	return crc;
}




/**
 * @brief Sends the request frame, including the calculated CRC.
 * @param len The length of the Modbus PDU (excluding ID, CRC).
 * @return HAL_StatusTypeDef
 */
static HAL_StatusTypeDef SendRequest(uint16_t len)
{
    uint16_t crc = MB_CalculateCRC16(mbMaster.txBuffer, len);
    mbMaster.txBuffer[len] = crc & 0xFF;         // CRC Low
    mbMaster.txBuffer[len + 1] = (crc >> 8) & 0xFF; // CRC High

    // DMA TX: the whole request frame is shifted out contiguously by the DMA
    // engine, independent of RTOS/ISR load, so no inter-character gaps can make
    // the downstream servo's RTU framing split the request. The caller waits for
    // MB_MASTER_TXCPLT_FLAG (set from HAL_UART_TxCpltCallback after the last bit).
    return HAL_UART_Transmit_DMA(mbMaster.huart, mbMaster.txBuffer, len + 2);

}


/**
 * @brief Executes the transaction: sends request, waits for response, validates response.
 * @param slaveId The slave ID.
 * @param txPDUlen The length of the Modbus Request PDU (e.g., 6 for FC03/06).
 * @param expectedFC The function code expected in the response.
 * @param expectedDataLen The expected length of the data field (or PDU len for echo responses).
 */
static MB_Status_t ExecuteTransaction(uint8_t slaveId, uint16_t txPDUlen, uint8_t expectedFC, uint16_t expectedDataLen)
{
    MB_Status_t status = MB_OK;

	HAL_StatusTypeDef st = HAL_UARTEx_ReceiveToIdle_DMA(mbMaster.huart, mbMaster.rxDMABuffer, MB_RX_BUFFER_SIZE);
	if (st == HAL_OK) {
		// Disable Half-Transfer Interrupt (we only care about Idle)
		__HAL_DMA_DISABLE_IT(mbMaster.huart->hdmarx, DMA_IT_HT);
	} else {
		return MB_ERROR_HAL;
	}

    // Register the calling task BEFORE starting the transfer, so both the
    // TX-complete callback and the RX-event callback can signal it.
    mbMaster.callingTaskHandle = osThreadGetId();
    osThreadFlagsClear(MB_MASTER_RESP_FLAG | MB_MASTER_TXCPLT_FLAG);

    // 1. Send the Request (DMA, non-blocking)
    if (SendRequest(txPDUlen) != HAL_OK) {
        mbMaster.callingTaskHandle = NULL;
        return MB_ERROR_TIMEOUT; // Or specific HAL error if desired
    }

    // 1b. Wait until the request frame has been fully transmitted.
    uint32_t txFlags = osThreadFlagsWait(MB_MASTER_TXCPLT_FLAG, osFlagsWaitAny, MB_DEFAULT_TIMEOUT_MS);
    if (!(txFlags & MB_MASTER_TXCPLT_FLAG)) {
    	HAL_UART_AbortTransmit_IT(mbMaster.huart);
        mbMaster.callingTaskHandle = NULL;
        return MB_ERROR_TIMEOUT;
    }

    // 2. Wait for response signal (set by MB_UART_HandleRxEvent)
    uint32_t flags = osThreadFlagsWait(MB_MASTER_RESP_FLAG, osFlagsWaitAny, MB_DEFAULT_TIMEOUT_MS);
    mbMaster.callingTaskHandle = NULL; // Clear handle

    if (!(flags & MB_MASTER_RESP_FLAG)) {
    	HAL_UART_AbortReceive_IT(mbMaster.huart);
        return MB_ERROR_TIMEOUT;
    }

    // --- Response Validation ---

    // Check minimum length (ID, FC, Data/ExcepCode, CRC_L, CRC_H)
    if (mbMaster.rxFrameLen < 5) return MB_ERROR_WRONG_RESPONSE;

    // Check CRC
    uint16_t receivedCRC = (mbMaster.rxFrameBuffer[mbMaster.rxFrameLen - 1] << 8) | mbMaster.rxFrameBuffer[mbMaster.rxFrameLen - 2];
    uint16_t calculatedCRC = MB_CalculateCRC16(mbMaster.rxFrameBuffer, mbMaster.rxFrameLen - 2);
    if (receivedCRC != calculatedCRC) {
    	return MB_ERROR_CRC;
    }

    // Check Slave ID
    if (mbMaster.rxFrameBuffer[0] != slaveId) {
    	return MB_ERROR_WRONG_RESPONSE;
    }

    // Check Function Code (Exception or Normal)
    uint8_t receivedFC = mbMaster.rxFrameBuffer[1];

    if (receivedFC == (expectedFC | 0x80)) {
        // Exception response: [ID][FC|0x80][ExceptionCode][CRC]
        return MB_ERROR_SLAVE_EXCEP;
    } else if (receivedFC != expectedFC) {
        return MB_ERROR_WRONG_RESPONSE;
    }

    // Check Expected Data Length (Specific to FC 0x03)
    if (expectedFC == MB_FC_READ_HOLDING_REGS) {
        // For FC 0x03, byte 2 is the Byte Count.
        if (mbMaster.rxFrameBuffer[2] != expectedDataLen) {
        	return MB_ERROR_WRONG_RESPONSE;
        }
    }
    // For FC 0x06 and 0x10 response (PDU length check)
    else if (mbMaster.rxFrameLen - 2 != expectedDataLen) {
        return MB_ERROR_WRONG_RESPONSE;
    }

    return status;
}



MB_Status_t MB_ReadHoldingRegs(uint8_t slaveId, uint16_t startAddr, uint16_t quantity, uint16_t *destArray)
{
	if (osMutexAcquire(mbMaster.txMutex, osWaitForever) != osOK) {
		return MB_ERROR_BUSY;
	}

	MB_Status_t status = MB_ERROR_WRONG_RESPONSE;
	uint16_t txPDUlen = 6; // PDU length for FC03 request

	// 1. Build Request Frame
	mbMaster.txBuffer[0] = slaveId;
	mbMaster.txBuffer[1] = MB_FC_READ_HOLDING_REGS;
	mbMaster.txBuffer[2] = (startAddr >> 8) & 0xFF;
	mbMaster.txBuffer[3] = startAddr & 0xFF;
	mbMaster.txBuffer[4] = (quantity >> 8) & 0xFF;
	mbMaster.txBuffer[5] = quantity & 0xFF;

	// Expected response data length (2 bytes per register)
	uint16_t expectedByteCount = quantity * 2;

	// 2. Execute Transaction
	status = ExecuteTransaction(slaveId, txPDUlen, MB_FC_READ_HOLDING_REGS, expectedByteCount);

	if (status == MB_OK) {
		// 3. Parse Data (starts at offset 3: [ID][FC][Count]...)
		uint8_t *pData = &(mbMaster.rxFrameBuffer[3]);
		for (int i = 0; i < quantity; i++) {
			// Modbus is Big Endian (MSB first)
			destArray[i] = (pData[i*2] << 8) | pData[i*2 + 1];
		}
	}

	osMutexRelease(mbMaster.txMutex);
	return status;
}


MB_Status_t MB_WriteSingleReg(uint8_t slaveId, uint16_t regAddr, uint16_t value)
{
	if (osMutexAcquire(mbMaster.txMutex, osWaitForever) != osOK) {
		return MB_ERROR_BUSY;
	}

    uint16_t txPDUlen = 6; // PDU length for FC06 request
    uint16_t responsePDUlen = 6; // PDU length for FC06 response (echo)

	// 1. Build Request Frame
	mbMaster.txBuffer[0] = slaveId;
	mbMaster.txBuffer[1] = MB_FC_WRITE_SINGLE_REG;
	mbMaster.txBuffer[2] = (regAddr >> 8) & 0xFF;
	mbMaster.txBuffer[3] = regAddr & 0xFF;
	mbMaster.txBuffer[4] = (value >> 8) & 0xFF;   // Value MSB
	mbMaster.txBuffer[5] = value & 0xFF;          // Value LSB

	// 2. Execute Transaction
	MB_Status_t status = ExecuteTransaction(slaveId, txPDUlen, MB_FC_WRITE_SINGLE_REG, responsePDUlen);

	osMutexRelease(mbMaster.txMutex);
	return status;
}



MB_Status_t MB_WriteMultipleRegs(uint8_t slaveId, uint16_t startAddr, uint16_t quantity, uint16_t *sourceArray)
{
	if (osMutexAcquire(mbMaster.txMutex, osWaitForever) != osOK) {
		return MB_ERROR_BUSY;
	}

	// 1. Build Request Frame
	uint8_t byteCount = quantity * 2;
	uint16_t txPDUlen = 7 + byteCount; // 7 bytes header + data
    uint16_t responsePDUlen = 6; // PDU length for FC10 response

    mbMaster.txBuffer[0] = slaveId;
    mbMaster.txBuffer[1] = MB_FC_WRITE_MULTI_REGS;
    mbMaster.txBuffer[2] = (startAddr >> 8) & 0xFF;
    mbMaster.txBuffer[3] = startAddr & 0xFF;
    mbMaster.txBuffer[4] = (quantity >> 8) & 0xFF;
    mbMaster.txBuffer[5] = quantity & 0xFF;
    mbMaster.txBuffer[6] = byteCount;

	uint8_t *pData = &(mbMaster.txBuffer[7]);
	for (int i = 0; i < quantity; i++) {
		pData[i*2]     = (sourceArray[i] >> 8) & 0xFF; // MSB
		pData[i*2 + 1] = sourceArray[i] & 0xFF;      // LSB
	}

	// 2. Execute Transaction
	MB_Status_t status = ExecuteTransaction(slaveId, txPDUlen, MB_FC_WRITE_MULTI_REGS, responsePDUlen);

	osMutexRelease(mbMaster.txMutex);
	return status;
}

MB_Status_t MB_ReadOneReg(uint8_t slaveId, uint16_t startAddr, uint16_t *data)
{
	MB_Status_t st = MB_ReadHoldingRegs(slaveId, startAddr, 1, data);
	return st;
}

/* 32 Bits Data operation */
MB_Status_t MB_Read32BitsWord(uint8_t slaveId, uint16_t startAddr,  uint32_t *data)
{
	uint16_t vals[2];
	MB_Status_t st = MB_ReadHoldingRegs(slaveId, startAddr, 2, vals);
	if (st != MB_OK) {
		return st;
	}
	*data = ((uint32_t)vals[0] << 16) | vals[1];
	return MB_OK;
}



MB_Status_t MB_Write32BitsWord(uint8_t slaveId, uint16_t startAddr, uint32_t val)
{
	uint16_t vals[2];
	vals[0] = (val >> 16) & 0x0000FFFF;
	vals[1] = (val & 0x0000FFFF);

	return MB_WriteMultipleRegs(slaveId, startAddr, 2, vals);
}

/* For called in HAL_UARTEx_RxEventCallback() */
void MB_UART_HandleRxEvent(UART_HandleTypeDef *huart, uint16_t size)
{
	if (huart->Instance == mbMaster.huart->Instance) {
		if (mbMaster.callingTaskHandle != NULL) {
			/* 1. Determine Frame Length */
			mbMaster.rxFrameLen = size;

			/* 2. Copy received data from DMA buffer to transaction buffer */
			memcpy(mbMaster.rxFrameBuffer, mbMaster.rxDMABuffer, mbMaster.rxFrameLen);

			/* 3. Signal the waiting application task */
			osThreadFlagsSet(mbMaster.callingTaskHandle, MB_MASTER_RESP_FLAG);
		}
	}
}


/**
 * @brief Public interface for HAL_UART_TxCpltCallback (master side).
 * Signals the calling task that the request frame has been fully transmitted.
 * Must be called inside HAL_UART_TxCpltCallback.
 */
void MB_UART_HandleTxCplt(UART_HandleTypeDef *huart)
{
	if (huart->Instance == mbMaster.huart->Instance && mbMaster.callingTaskHandle != NULL) {
		osThreadFlagsSet(mbMaster.callingTaskHandle, MB_MASTER_TXCPLT_FLAG);
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



void MB_Master_StartTask(void)
{
    const osThreadAttr_t taskAttr = {
        .name = "MB_Master",
        .stack_size = 256,
        .priority = (osPriority_t) osPriorityNormal,
    };
    mbMaster.taskHandle = osThreadNew(MB_Master_Task, NULL, &taskAttr);
}


/**
 * @brief UART error handler for the Master link.
 * Must be called from HAL_UART_ErrorCallback() for the master UART.
 *
 * Only clears the error - ExecuteTransaction() re-arms the RX DMA at the start
 * of every transaction, so restarting it here would race with the next request.
 */
void MB_UART_HandleError(UART_HandleTypeDef *huart)
{
	if (mbMaster.huart == NULL || huart->Instance != mbMaster.huart->Instance) {
		return;
	}
	mbMasterErrCount++;
	mbMasterErrCode |= huart->ErrorCode;
	__HAL_UART_CLEAR_PEFLAG(huart);
	huart->ErrorCode = HAL_UART_ERROR_NONE;
}

void MB_Master_GetDiag(uint32_t *errCount, uint32_t *errCode)
{
	if (errCount) *errCount = mbMasterErrCount;
	if (errCode)  *errCode  = mbMasterErrCode;
}
