/*
 * modbus_slave.c
 *
 *  Created on: 2025年11月26日
 *      Author: ranfa
 */

#include "modbus_slave.h"
#include <string.h>
#include "bsp_uart.h"
#include "debug_shell.h"

extern void MB_Local_RegInit(void);
extern void MB_UpdateStatus(void);
extern void MB_CommandParse(void);

// --- External Master Context access (defined in modbus_master.c) ---
extern MB_MasterCtx_t mbMotor;
extern MB_MasterCtx_t mbMFC;
extern MB_MasterCtx_t mbPZT;
extern MB_MasterCtx_t *getMBMasterCtx(target_t target); // Forward declaration

static MB_SlaveCtx_t mbSlave;

//static uint8_t mbSlaveRxBuf[MB_SLAVE_RX_BUFFER_SIZE];
//static uint8_t mbSlaveRxLen = 0;

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
 * @brief Determines the target device based on the starting register address.
 * @param startAddr Modbus address (1-based, 4xxxx)
 * @param quantity Number of registers
 * @return target_t The determined device target.
 */
static target_t get_target_from_addr(uint16_t startAddr, uint16_t quantity)
{
    // Convert 4xxxx address to 0-based
    uint16_t endAddr = startAddr + quantity - 1;
    // Check Local Block: 0 to MB_REG_LOCAL_COUNT - 1
    if (endAddr <= MB_LOCAL_REG_START+MB_REG_LOCAL_COUNT-1 && startAddr >= MB_LOCAL_REG_START) {
        return TARGET_LOCAL;
    }

    // Check Motor Block: MB_REG_LOCAL_COUNT to MB_REG_LOCAL_COUNT + MB_REG_BLOCK_COUNT - 1
    if (startAddr >= MB_MOTOR_REG_START && endAddr <= MB_MOTOR_REG_START + MB_REG_BLOCK_COUNT - 1) {
        return TARGET_MOTOR;
    }

    // Check MFC Block
    if (startAddr >= MB_MFC_REG_START && endAddr <= MB_MFC_REG_START + MB_REG_BLOCK_COUNT - 1) {
        return TARGET_MFC;
    }

    // Check PZT Block
    if (startAddr >= MB_PZT_REG_START && endAddr <= MB_PZT_REG_START + MB_REG_BLOCK_COUNT - 1) {
        return TARGET_PZT;
    }

    return TARGET_INVALID;
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
static uint16_t sentCount = 0;
static uint16_t processCount = 0;
static void MB_Slave_Task(void *argument)
{
	MB_FrameMsg_t msg;

	while (1) {
		// Wait for a new frame from the UART RX callback
		if (osMessageQueueGet(mbSlave.rxQueue, &msg, NULL, osWaitForever) == osOK) {
			processCount++;
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
			uint16_t quantity = 1; // Only relevant for FC03/10

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

			// 3. Determine Target (Local or Remote)
			target_t target = get_target_from_addr(startAddr, quantity);

			if (target == TARGET_INVALID) {
			    SendException(&mbSlave, fc, MB_EX_ILLEGAL_DATA_ADDR);
			    continue;
			}

			// 4. Process Request
			if (target == TARGET_LOCAL) {
			    // Handle Local Register Access
			    uint16_t local_idx = startAddr - MB_LOCAL_REG_START;

			    if (local_idx + quantity > MB_REG_LOCAL_COUNT) {
			        SendException(&mbSlave, fc, MB_EX_ILLEGAL_DATA_ADDR);
			        continue;
			    }

			    switch (fc) {
			        case MB_FC_READ_HOLDING_REGS:
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
			    // --- GATEWAY MODE: Forward to Remote Master ---
			    MB_Status_t master_status = MB_ERROR_WRONG_RESPONSE;
//			    uint16_t remote_start_addr = 0; // For re-mapping if needed, but keeping it 0-based for simplicity
//
//			    // Calculate the 0-based index offset within the remote device's block
//			    if (target == TARGET_MOTOR) {
//			        remote_start_addr = startAddr - MB_MOTOR_REG_START;
//			    } else if (target == TARGET_MFC) {
//			        remote_start_addr = startAddr - MB_MFC_REG_START;
//			    } else if (target == TARGET_PZT) {
//			        remote_start_addr = startAddr - MB_PZT_REG_START;
//			    }

			    // The remote device may not use 40001 addressing, so we remap to 0-based (or 1-based, depending on device)
			    // For this example, we assume the remote device uses 0-based registers.

			    // Copy response buffers
			    uint16_t dest_regs[MB_REG_BLOCK_COUNT];
			    uint16_t source_regs[MB_REG_BLOCK_COUNT];
			    MB_SlaveCtx_t *ctx = &mbSlave;
			    switch (fc) {
			        case MB_FC_READ_HOLDING_REGS:
			            master_status = MB_ReadHoldingRegs(target, startAddr, quantity, dest_regs);
			            if (master_status == MB_OK) {
			                // Build successful response from the master's response data
			                ctx->txBuffer[0] = ctx->addr;
			                ctx->txBuffer[1] = MB_FC_READ_HOLDING_REGS;
			                ctx->txBuffer[2] = (uint8_t)(quantity * 2);
			                uint8_t *pData = &ctx->txBuffer[3];
			                for (int i = 0; i < quantity; i++) {
			                    pData[i*2] = (dest_regs[i] >> 8) & 0xFF;
			                    pData[i*2 + 1] = dest_regs[i] & 0xFF;
			                }
			                SendResponse(ctx, 2 + (quantity * 2));
			            }
			            break;

			        case MB_FC_WRITE_SINGLE_REG:
			            master_status = MB_WriteSingleReg(target, startAddr, (rxFrame[4] << 8) | rxFrame[5]);
			            if (master_status == MB_OK) {
			                // Forward the echo response from the slave UART
			                ctx->txBuffer[0] = ctx->addr;
			                memcpy(&ctx->txBuffer[1], &rxFrame[1], 5); // FC + Addr + Value
			                SendResponse(ctx, 5);
			            }
			            sentCount++;
			            break;

			        case MB_FC_WRITE_MULTI_REGS:
			            // Parse data from HMI request into source_regs array
			            const uint8_t *pReqData = &rxFrame[7];
			            for (int i = 0; i < quantity; i++) {
			                source_regs[i] = (pReqData[i*2] << 8) | pReqData[i*2 + 1];
			            }

			            master_status = MB_WriteMultipleRegs(target, startAddr, quantity, source_regs);
			            if (master_status == MB_OK) {
			                // Forward the short successful response from the slave UART
			                ctx->txBuffer[0] = ctx->addr;
			                memcpy(&ctx->txBuffer[1], &rxFrame[1], 5); // FC + StartAddr + Quantity
			                SendResponse(ctx, 5);
			            }
			            break;
			    }

			    // If master transaction failed, send appropriate exception to HMI
			    if (master_status != MB_OK) {
			        uint8_t ex_code = (target << 4) |(master_status + 4);//MB_EX_SLAVE_DEVICE_FAILURE; // Default for timeout/internal errors
			        if (master_status == MB_ERROR_SLAVE_EXCEP) {
			            // Note: If the Master returns EXCEP, we need the exact code.
			            // Since MB_Master_... currently only returns MB_ERROR_SLAVE_EXCEP,
			            // we default to 4 (Slave Failure) or assume the Master needs to provide
			            // the actual exception code via its own context/return value.
			            // For simplicity, we assume the Master handles it and we send a generic Slave Failure.
			        }
			        SendException(&mbSlave, fc, ex_code);
			    }
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
	mbSlave.huart = &MB_PC_UART_HANDLE;
	mbSlave.addr = MB_LOCAL_ADDR;

	// Initialize local registers (optional: set to zero or known values)
//	memset(mbSlave.local_registers, 0, sizeof(mbSlave.local_registers));
//	mbSlave.local_registers[0] = 0xAA; // Example value
	MB_Local_RegInit();

	// 2. Setup RTOS Queue
	const osMessageQueueAttr_t queueAttr = {.name = "mbSlaveQueue"};
	mbSlave.rxQueue = osMessageQueueNew(MB_SLAVE_QUEUE_DEPTH, sizeof(MB_FrameMsg_t), &queueAttr);

	// 3. Setup Master Context References
	mbSlave.mbMotor = getMBMasterCtx(TARGET_MOTOR);
	mbSlave.mbMFC = getMBMasterCtx(TARGET_MFC);
	mbSlave.mbPZT = getMBMasterCtx(TARGET_PZT);

	// 4. Start DMA reception
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
        .stack_size = 256 * 4, // Increased stack size for gateway/master calls
        .priority = (osPriority_t) osPriorityNormal,
    };
    mbSlave.taskHandle = osThreadNew(MB_Slave_Task, NULL, &mbSlaveTask_attributes);
    if (mbSlave.taskHandle == NULL) {
    	LOG_ERROR("Create modbus slave thread FAILED!");
    } else {
    	LOG_INFO("Create modbus slave thread OK");
    }
}


/**
 * @brief UART Rx Event Handler for the Slave/Gateway
 * This must be called from HAL_UARTEx_RxEventCallback(huart6, size)
 */
static uint16_t itCount=0;
static uint16_t fLength = 0;
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
			fLength = size;
		}

		// Re-start DMA for continuous monitoring (HAL does this, but good practice to ensure)
        HAL_UARTEx_ReceiveToIdle_DMA(huart, mbSlave.rxDMABuffer, MB_SLAVE_RX_BUFFER_SIZE);
	}
}
