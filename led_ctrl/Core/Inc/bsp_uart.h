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
 * 本板只有一个串口，它按 USART1_ROLE 扮演两种角色之一。两个宏刻意指向同一个句柄，
 * 是为了让上层代码读起来和另外四块板一致（那几块板上这是两条不同的物理链路）。 */
#if   USART1_ROLE == USART1_ROLE_SHELL
#define		DEBUG_SHELL_UART_HANDLE		(huart1)	// uart for debug shell
#elif USART1_ROLE == USART1_ROLE_MODBUS
#define		AD_COMM_UART_HANDLE			(huart1)	// uart for modbus communication with adc_comm
#endif

#endif /* INC_BSP_UART_H_ */
