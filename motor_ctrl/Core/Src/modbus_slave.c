/*
 * modbus_slave.c
 *
 *  Created on: 2025年12月4日
 *      Author: ranfa
 */
#include "modbus_slave.h"
#include "bsp_uart.h"
#include "string.h"
#include "debug_shell.h"

static MB_SlaveCtx_t mbSlave;
/* --- Slave link diagnostics -------------------------------------------------
 * A UART line error aborts the RX DMA inside HAL (UART_EndRxTransfer), and
 * nothing used to re-arm it: one glitch killed reception permanently. These
 * counters, MB_SLAVE_UART_HandleError() and the task watchdog below make that
 * self-healing and let "mb -i" say what actually happened.
 * -------------------------------------------------------------------------*/
static volatile uint32_t mbSlaveErrCount  = 0;	/* HAL error callbacks seen */
static volatile uint32_t mbSlaveErrCode   = 0;	/* OR of every HAL_UART_ERROR_* bit */
static volatile uint32_t mbSlaveRearmFail = 0;	/* re-arm refused inside the ISR */
static volatile uint32_t mbSlaveWdRearm   = 0;	/* re-arms done by the task watchdog */

/* Nothing received for this long -> make sure reception is still armed. */
#define MB_SLAVE_RX_WD_MS	(1000U)


/* Thread flag used to signal the slave task that a DMA response transfer
 * has fully completed (set from HAL_UART_TxCpltCallback). */
#define MB_SLAVE_TXCPLT_FLAG    0x01U
#define MB_SLAVE_TX_TIMEOUT_MS  100U

extern void MB_Local_RegInit(void);
extern void MB_UpdateStatus(void);
extern void MB_CommandParse(void);

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
 * @brief Sends a response frame back to the HMI via the slave UART.
 */
static HAL_StatusTypeDef SendResponse(MB_SlaveCtx_t *ctx, uint16_t pduLen)
{
    // Frame length = ID (1) + PDU (pduLen) + CRC (2)
    uint16_t frameLen = 1 + pduLen + 2;
    uint16_t crc = MB_CalculateCRC16(ctx->txBuffer, frameLen - 2);

    ctx->txBuffer[frameLen - 2] = crc & 0xFF;         // CRC Low
    ctx->txBuffer[frameLen - 1] = (crc >> 8) & 0xFF; // CRC High

    /* Clear any stale TX-complete flag before starting a new transfer. */
    osThreadFlagsClear(MB_SLAVE_TXCPLT_FLAG);

    /* DMA TX: the whole frame is shifted out contiguously by the DMA engine,
     * independent of RTOS task scheduling. This removes the inter-character
     * gaps that a blocking, task-context HAL_UART_Transmit could introduce
     * when preempted, which otherwise trip the master's UART IDLE-line
     * detection and split one response into several short frames. */
    HAL_StatusTypeDef st = HAL_UART_Transmit_DMA(ctx->huart, ctx->txBuffer, frameLen);
    if (st != HAL_OK) {
        return st;
    }

    /* Block until the transfer really completes (signalled from
     * HAL_UART_TxCpltCallback) so txBuffer is not reused while still in flight. */
    uint32_t flags = osThreadFlagsWait(MB_SLAVE_TXCPLT_FLAG, osFlagsWaitAny, MB_SLAVE_TX_TIMEOUT_MS);
    if (flags & osFlagsError) {
        HAL_UART_AbortTransmit(ctx->huart);
        return HAL_TIMEOUT;
    }
    return HAL_OK;
}

/**
 * @brief Sends an exception response back to the HMI.
 */
static void SendException(MB_SlaveCtx_t *ctx, uint8_t fc, uint8_t exceptionCode)
{
    // [ID][FC|0x80][ExceptionCode]
    ctx->txBuffer[0] = ctx->addr;
    ctx->txBuffer[1] = fc | 0x80;
    ctx->txBuffer[2] = exceptionCode;
    // PDU length is 2 (FC + ExceptionCode)
    SendResponse(ctx, 2);
}

/**
 * @brief Processes Modbus Read Holding Registers (FC 0x03) locally.
 */
static void ProcessReadHoldingRegsLocal(MB_SlaveCtx_t *ctx, uint16_t startAddr, uint16_t quantity)
{
    uint16_t local_idx = startAddr - MB_LOCAL_REG_START;

    // [ID][FC][ByteCount][Data...][CRC]
    ctx->txBuffer[0] = ctx->addr;
    ctx->txBuffer[1] = MB_FC_READ_HOLDING_REGS;
    ctx->txBuffer[2] = (uint8_t)(quantity * 2); // Byte Count

    uint8_t *pData = &ctx->txBuffer[3];
    uint16_t value = 0;
    for (int i = 0; i < quantity; i++) {
        // Modbus is Big Endian (MSB first)
    	if (ctx->local_registers[local_idx + i] == NULL) {
    		SendException(ctx, MB_FC_READ_HOLDING_REGS, MB_EX_ILLEGAL_DATA_ADDR);
			return;
    	}
    	value = *(ctx->local_registers[local_idx + i]);
        pData[i*2] = (value >> 8) & 0xFF;
        pData[i*2 + 1] = value & 0xFF;
    }
    // PDU length: FC (1) + ByteCount (1) + Data (2*quantity)
    SendResponse(ctx, 2 + (quantity * 2));
}

/**
 * @brief Processes Modbus Write Single Register (FC 0x06) locally.
 */
static void ProcessWriteSingleRegLocal(MB_SlaveCtx_t *ctx, uint16_t regAddr, uint16_t value)
{
    uint16_t local_idx = regAddr - MB_LOCAL_REG_START;
    // [ID][FC][ByteCount][Data...][CRC]
    ctx->txBuffer[0] = ctx->addr;
    ctx->txBuffer[1] = MB_FC_WRITE_SINGLE_REG;
    ctx->txBuffer[2] = (uint8_t)(regAddr >> 8) & 0xFF;
    ctx->txBuffer[3] = (uint8_t)(regAddr & 0xFF);
    ctx->txBuffer[4] = (value >> 8) & 0xFF;
    ctx->txBuffer[5] = value & 0xFF;
    if (ctx->local_registers[local_idx] == NULL) {
    	SendException(ctx, MB_FC_WRITE_SINGLE_REG, MB_EX_ILLEGAL_DATA_ADDR);
    	return;
    }
    *(ctx->local_registers[local_idx]) = value;


    // Response is echo of request: [ID][FC][RegAddr][Value][CRC]
    // The request frame contains all necessary echo data in bytes 0-5.
    SendResponse(ctx, 5); // PDU length is 5 (FC + Addr + Value)
}

/**
 * @brief Processes Modbus Write Multiple Registers (FC 0x10) locally.
 * Request PDU: [ID][10][StartAddr][Quantity][ByteCount][Data...][CRC]
 */
static void ProcessWriteMultipleRegsLocal(MB_SlaveCtx_t *ctx, const uint8_t *rxFrame, uint16_t startAddr, uint16_t quantity)
{
    uint16_t local_idx = startAddr - MB_LOCAL_REG_START;
    const uint8_t *pData = &rxFrame[7]; // Data starts after ByteCount

    for (int i = 0; i < quantity; i++) {
        // Modbus is Big Endian
    	if (ctx->local_registers[local_idx] == NULL) {
			SendException(ctx, MB_FC_WRITE_MULTI_REGS, MB_EX_ILLEGAL_DATA_ADDR);
			return;
		}
        uint16_t value = (pData[i*2] << 8) | pData[i*2 + 1];
        *(ctx->local_registers[local_idx + i]) = value;
    }

    // Response is short: [ID][10][StartAddr][Quantity][CRC] (PDU Length 5)
    ctx->txBuffer[0] = ctx->addr;
    ctx->txBuffer[1] = MB_FC_WRITE_MULTI_REGS;
    memcpy(&ctx->txBuffer[2], &rxFrame[2], 4); // Copy StartAddr and Quantity from request

    SendResponse(ctx, 5); // PDU length is 5 (FC + Addr + Quantity)
}

/**
 * @brief Modbus Slave/Gateway Main Task
 */
static uint16_t processCount = 0;
static uint16_t validCount = 0;
static void MB_Slave_Task(void *argument)
{
	MB_FrameMsg_t msg;

	while (1) {
		// Wait for a new frame from the UART RX callback
		if (osMessageQueueGet(mbSlave.rxQueue, &msg, NULL, MB_SLAVE_RX_WD_MS) == osOK) {
			const uint8_t *rxFrame = msg.frame;
			uint16_t rxLen = msg.length;
			processCount++;

			// 0. Preliminary Check (Min length and Slave ID)
			if (rxLen < 8 || rxFrame[0] != MB_LOCAL_ADDR) {
				continue; // Not for this slave or too short
			}

			// 1. Check CRC
			uint16_t receivedCRC = (rxFrame[rxLen - 1] << 8) | rxFrame[rxLen - 2];
			uint16_t calculatedCRC = MB_CalculateCRC16(rxFrame, rxLen - 2);
			if (receivedCRC != calculatedCRC) {
				continue; // Invalid frame, drop it
			}

			// 2. Extract PDU components
			uint8_t fc = rxFrame[1];
			uint16_t startAddr = (rxFrame[2] << 8) | rxFrame[3];
			uint16_t quantity = 0; // Only relevant for FC03/10

			if (fc == MB_FC_READ_HOLDING_REGS || fc == MB_FC_WRITE_MULTI_REGS) {
				quantity = (rxFrame[4] << 8) | rxFrame[5];
				// Check if Data Length matches ByteCount for FC10
				if (fc == MB_FC_WRITE_MULTI_REGS) {
					uint8_t byteCount = rxFrame[6];
					if (byteCount != quantity * 2) {
						SendException(&mbSlave, fc, MB_EX_ILLEGAL_DATA_VALUE);
						continue;
					}
				}
			} else if (fc == MB_FC_WRITE_SINGLE_REG) {
				// RegAddr is in bytes 2-3, Value is in 4-5.
			} else {
				// Function code not supported
				SendException(&mbSlave, fc, MB_EX_ILLEGAL_FUNCTION);
				continue;
			}


			// 3. Process Request
			// Handle Local Register Access
			uint16_t local_idx = startAddr - MB_LOCAL_REG_START;

			if (local_idx + quantity > MB_REG_LOCAL_COUNT) {
				SendException(&mbSlave, fc, MB_EX_ILLEGAL_DATA_ADDR);
				continue;
			}
			validCount++;
			switch (fc) {
			case MB_FC_READ_HOLDING_REGS:
				MB_UpdateStatus();
				ProcessReadHoldingRegsLocal(&mbSlave, startAddr, quantity);
				break;
			case MB_FC_WRITE_SINGLE_REG:
				ProcessWriteSingleRegLocal(&mbSlave, startAddr, (rxFrame[4] << 8) | rxFrame[5]);
				MB_CommandParse();
				break;
			case MB_FC_WRITE_MULTI_REGS:
				ProcessWriteMultipleRegsLocal(&mbSlave, rxFrame, startAddr, quantity);
				MB_CommandParse();
				break;
			}
		} else {
			/* Watchdog: nothing arrived for a while. When reception is healthy
			 * RxState is BUSY_RX and this does nothing; if an error path (or a
			 * re-arm that lost the handle lock) left it disarmed, restart it. */
			if (mbSlave.huart != NULL &&
				mbSlave.huart->RxState != HAL_UART_STATE_BUSY_RX) {
				mbSlaveWdRearm++;
				__HAL_UART_CLEAR_PEFLAG(mbSlave.huart);
				mbSlave.huart->ErrorCode = HAL_UART_ERROR_NONE;
				HAL_UARTEx_ReceiveToIdle_DMA(mbSlave.huart, mbSlave.rxDMABuffer,
											 MB_SLAVE_RX_BUFFER_SIZE);
			}
		}
	}
}




/**
 * @brief Initializes the Slave context and RTOS components.
 */
void MB_Slave_Init()
{
	// 1. Initialize Context
	mbSlave.huart = &AD_COMM_UART_HANDLE;
	mbSlave.addr = MB_LOCAL_ADDR;

	// Initialize local registers (optional: set to zero or known values)
//	memset(mbSlave.local_registers, 0, sizeof(mbSlave.local_registers));
//	mbSlave.local_registers[0] = 0xAA; // Example value
	MB_Local_RegInit();

	// 2. Setup RTOS Queue
	const osMessageQueueAttr_t queueAttr = {.name = "mbSlaveQueue"};
	mbSlave.rxQueue = osMessageQueueNew(MB_SLAVE_QUEUE_DEPTH, sizeof(MB_FrameMsg_t), &queueAttr);

	// 3. Start DMA reception
	HAL_UARTEx_ReceiveToIdle_DMA(mbSlave.huart, mbSlave.rxDMABuffer, MB_SLAVE_RX_BUFFER_SIZE);
}


void MB_Slave_DefineReg(uint16_t idx, uint16_t *valP)
{
	if (idx >= MB_REG_LOCAL_COUNT) {
		return;
	}
	mbSlave.local_registers[idx] = valP;
}

/**
 * @brief Starts the Modbus Slave Task.
 */
void MB_Slave_StartTask(void)
{
    const osThreadAttr_t mbSlaveTask_attributes = {
        .name = "ModbusSlave",
        .stack_size = 512,
        .priority = (osPriority_t) osPriorityNormal,
    };
    mbSlave.taskHandle = osThreadNew(MB_Slave_Task, NULL, &mbSlaveTask_attributes);
    if (mbSlave.taskHandle == NULL) {
    	LOG_ERROR("Create MODBUS slave thread FAILED!");
    } else {
    	LOG_INFO("Create MODBUS slave thread OK");
    }
}


/**
 * @brief UART Rx Event Handler for the Slave/Gateway
 * This must be called from HAL_UARTEx_RxEventCallback(huart6, size)
 */
static uint16_t itCount = 0;

void MB_SLAVE_UART_HandleRxEvent(UART_HandleTypeDef *huart, uint16_t size)
{
	if (huart->Instance == mbSlave.huart->Instance) {

		// Stop DMA and restart it after processing in the task if needed, but
		// HAL_UARTEx_ReceiveToIdle_DMA automatically restarts, so we just copy and notify.

		MB_FrameMsg_t msg;

		if (size > 0 && size <= MB_MAX_FRAME_SIZE) {
			msg.length = size;
			// Copy frame from DMA buffer
			memcpy(msg.frame, mbSlave.rxDMABuffer, size);

			// Send message to the Slave Task
			osMessageQueuePut(mbSlave.rxQueue, &msg, 0, 0);
			itCount++;
		}

		// Re-start DMA for continuous monitoring (HAL does this, but good practice to ensure)
        HAL_UARTEx_ReceiveToIdle_DMA(huart, mbSlave.rxDMABuffer, MB_SLAVE_RX_BUFFER_SIZE);
	}
}


/**
 * @brief UART TX-complete handler for the Slave.
 * Must be called from HAL_UART_TxCpltCallback() for the slave UART.
 * Signals the slave task that the DMA response transfer has finished.
 */
void MB_SLAVE_UART_HandleTxCplt(UART_HandleTypeDef *huart)
{
	if (huart->Instance == mbSlave.huart->Instance && mbSlave.taskHandle != NULL) {
		osThreadFlagsSet(mbSlave.taskHandle, MB_SLAVE_TXCPLT_FLAG);
	}
}


/**
 * @brief UART error handler for the Slave link.
 * Must be called from HAL_UART_ErrorCallback() for the slave UART.
 *
 * On PE/FE/NE/ORE the HAL aborts the RX DMA and leaves RxState READY, so the
 * reception has to be started again here or the link stays deaf forever.
 */
void MB_SLAVE_UART_HandleError(UART_HandleTypeDef *huart)
{
	if (mbSlave.huart == NULL || huart->Instance != mbSlave.huart->Instance) {
		return;
	}

	mbSlaveErrCount++;
	mbSlaveErrCode |= huart->ErrorCode;

	/* Clear the sticky PE/FE/NE/ORE flags (SR read + DR read) so the freshly
	 * re-armed reception does not trip the same error again immediately. */
	__HAL_UART_CLEAR_PEFLAG(huart);
	huart->ErrorCode = HAL_UART_ERROR_NONE;

	/* Can still be refused with HAL_BUSY when the handle lock is held by a
	 * response TX in flight - the task watchdog picks that case up. */
	if (HAL_UARTEx_ReceiveToIdle_DMA(huart, mbSlave.rxDMABuffer,
									 MB_SLAVE_RX_BUFFER_SIZE) != HAL_OK) {
		mbSlaveRearmFail++;
	}
}

void MB_Slave_GetDiag(MB_SlaveDiag_t *diag)
{
	if (diag == NULL) {
		return;
	}
	diag->rxEvtCount   = itCount;
	diag->processCount = processCount;
	diag->validCount   = validCount;
	diag->errCount     = mbSlaveErrCount;
	diag->errCode      = mbSlaveErrCode;
	diag->rearmFail    = mbSlaveRearmFail;
	diag->wdRearm      = mbSlaveWdRearm;
	diag->rxState      = (mbSlave.huart != NULL) ? (uint8_t)mbSlave.huart->RxState : 0;
}

void MB_Slave_ClearDiag(void)
{
	itCount = 0;
	processCount = 0;
	validCount = 0;
	mbSlaveErrCount = 0;
	mbSlaveErrCode = 0;
	mbSlaveRearmFail = 0;
	mbSlaveWdRearm = 0;
}
