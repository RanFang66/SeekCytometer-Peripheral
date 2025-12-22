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

extern void MB_Local_RegInit(void);
extern void MB_UpdateStatus(void);
extern void MB_CommandParse(void);
extern void MB_UpdateCtrlParas(void);

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
static void SendResponse(MB_SlaveCtx_t *ctx, uint16_t pduLen)
{
    // Frame length = ID (1) + PDU (pduLen) + CRC (2)
    uint16_t frameLen = 1 + pduLen + 2;
    uint16_t crc = MB_CalculateCRC16(ctx->txBuffer, frameLen - 2);

    ctx->txBuffer[frameLen - 2] = crc & 0xFF;         // CRC Low
    ctx->txBuffer[frameLen - 1] = (crc >> 8) & 0xFF; // CRC High

    // Blocking TX on the slave UART
    HAL_UART_Transmit(ctx->huart, ctx->txBuffer, frameLen, 100);
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
static void MB_Slave_Task(void *argument)
{
	MB_FrameMsg_t msg;

	while (1) {
		// Wait for a new frame from the UART RX callback
		if (osMessageQueueGet(mbSlave.rxQueue, &msg, NULL, osWaitForever) == osOK) {
			const uint8_t *rxFrame = msg.frame;
			uint16_t rxLen = msg.length;

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

			switch (fc) {
			case MB_FC_READ_HOLDING_REGS:
				if (startAddr < MB_LOCAL_REG_START + 71) {
					MB_UpdateStatus();
				} else {
					MB_UpdateCtrlParas();
				}
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
		}

		// Re-start DMA for continuous monitoring (HAL does this, but good practice to ensure)
        HAL_UARTEx_ReceiveToIdle_DMA(huart, mbSlave.rxDMABuffer, MB_SLAVE_RX_BUFFER_SIZE);
	}
}
