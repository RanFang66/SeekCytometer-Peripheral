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
