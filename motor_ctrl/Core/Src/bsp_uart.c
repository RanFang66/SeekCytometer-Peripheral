/*
 * bsp_uart.c
 *
 *  Created on: 2025年11月27日
 *      Author: ranfa
 */


#include "bsp_uart.h"
#include "debug_shell.h"
#include "modbus_master.h"
#include "modbus_slave.h"

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	Shell_UartRecvCallBack(huart);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	MB_UART_HandleRxEvent(huart, Size);
	MB_SLAVE_UART_HandleRxEvent(huart, Size);
}
