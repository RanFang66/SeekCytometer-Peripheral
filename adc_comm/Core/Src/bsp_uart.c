/*
 * bsp_uart.c
 *
 *  Created on: 2025年11月26日
 *      Author: ranfa
 */


#include "bsp_uart.h"
#include "modbus_master.h"
#include "modbus_slave.h"
#include "debug_shell.h"


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	Shell_UartRecvCallBack(huart);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    // Handle Master Receives
    MB_UART_HandleRxEvent(huart, Size);

    // Handle Slave/Gateway Receive
    MB_SLAVE_UART_HandleRxEvent(huart, Size);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    // Master request TX (huart5/huart1/huart4) and slave response TX (huart6).
    // Each handler filters by its own UART instance, so both are safe to call.
    MB_UART_HandleTxCplt(huart);
    MB_SLAVE_UART_HandleTxCplt(huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	// A PE/FE/NE/ORE aborts the RX DMA inside the HAL. Each handler filters by
	// its own UART instance; the slave one restarts reception, the master one
	// only clears the error (it re-arms per transaction), and the shell one
	// restarts its single-byte IT reception.
	MB_UART_HandleError(huart);
	MB_SLAVE_UART_HandleError(huart);
	Shell_UartErrorCallBack(huart);
}
