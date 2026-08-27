/*
 * bsp_uart.c
 *
 *  Created on: 2025年11月27日
 *      Author: ranfa
 */


#include "bsp_uart.h"
#include "debug_shell.h"

#include "modbus_slave.h"

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	Shell_UartRecvCallBack(huart);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	MB_SLAVE_UART_HandleRxEvent(huart, Size);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	MB_SLAVE_UART_HandleTxCplt(huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	// A PE/FE/NE/ORE aborts the RX DMA inside the HAL. Each handler filters by
	// its own UART instance; the slave one restarts reception, the shell one
	// restarts its single-byte IT reception.
	MB_SLAVE_UART_HandleError(huart);
	Shell_UartErrorCallBack(huart);
}
