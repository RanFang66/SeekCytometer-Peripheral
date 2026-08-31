/*
 * bsp_uart.h
 *
 *  Created on: 2026年8月31日
 *      Author: ranfa
 */

#ifndef INC_BSP_UART_H_
#define INC_BSP_UART_H_

#include "usart.h"
#include "board_config.h"

/* BSP USART Handle function definition
 *
 * This board has a single USART that plays one of two roles, selected by
 * USART1_ROLE. Both macros deliberately resolve to the same handle so that
 * upper layers read the same as on the other four boards, where these are two
 * distinct physical links. */
#if   USART1_ROLE == USART1_ROLE_SHELL
#define		DEBUG_SHELL_UART_HANDLE		(huart1)	// uart for debug shell
#elif USART1_ROLE == USART1_ROLE_MODBUS
#define		AD_COMM_UART_HANDLE			(huart1)	// uart for modbus communication with adc_comm
#endif

#endif /* INC_BSP_UART_H_ */
