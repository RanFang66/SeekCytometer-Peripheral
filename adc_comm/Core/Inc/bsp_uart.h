/*
 * bsp_uart.h
 *
 *  Created on: 2025年11月26日
 *      Author: ranfa
 */

#ifndef INC_BSP_UART_H_
#define INC_BSP_UART_H_

#include "usart.h"

// BSP USART Handle function definition
// USART2/PA2-PA3 used to carry the debug shell. It now carries the Modbus link
// to the CS_LED_Ctrl board, and the shell moved to USART3/PB10-PB11, which is
// broken out on this board. Anything probing the old header will find Modbus
// traffic, not a prompt.
#define		DEBUG_SHELL_UART_HANDLE			huart3				// uart for debug shell
#define 	MB_PC_UART_HANDLE				huart6				// uart for modbus communication with PC HMI
#define  	MB_MOTOR_UART_HANDLE 			huart5				// uart for modbus communication with motor control
#define 	MB_MFC_UART_HANDLE				huart1				// uart for modbus communication with MFC
#define 	MB_PZT_UART_HANDLE				huart4				// uart for modbus communication with PZT





#endif /* INC_BSP_UART_H_ */
