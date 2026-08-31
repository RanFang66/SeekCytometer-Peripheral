/*
 * bsp_uart.c
 *
 *  Created on: 2026年8月31日
 *      Author: ranfa
 *
 *  HAL weak-callback dispatchers. Which handler gets the callbacks depends on
 *  what USART1 is doing in this build - see board_config.h.
 */

#include "bsp_uart.h"
#include "debug_shell.h"

/* Keep the translation unit non-empty in the Modbus configuration. */
typedef int bsp_uart_translation_unit_not_empty_t;

#if USART1_ROLE == USART1_ROLE_SHELL

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    Shell_UartRecvCallBack(huart);
}

/* A PE/FE/NE/ORE aborts the ongoing reception inside the HAL and leaves it
 * disarmed. Nothing re-arms it on its own, so implementing this callback is
 * mandatory - see doc/pitfall_notes/Modbus从站UART错误后接收永久停死.md, where
 * the missing implementation cost three boards their Modbus link. */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    Shell_UartErrorCallBack(huart);
}

#else /* USART1_ROLE == USART1_ROLE_MODBUS */

/* Filled in at M6, when modbus_slave.c lands. */

#endif /* USART1_ROLE */
