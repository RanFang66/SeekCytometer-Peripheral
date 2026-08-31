/*
 * board_config.h
 *
 *  Created on: 2026年8月31日
 *      Author: ranfa
 *
 *  Board-level compile-time switches for CS_LED_Ctrl V1.0.
 *
 *  The STM32F030F4P6 has exactly one USART. The PA9/PA10 alternate-function
 *  group is not a way out: PA9 is unconnected, PA10 is already the AUX_IN dry
 *  contact, and even wired up they are the same USART1 peripheral. So the debug
 *  shell and the Modbus slave cannot coexist - see
 *  doc/design/CS_LED_Lock_Ctrl_详细设计.md section 5.2.
 *
 *  The role comes from -DUSART1_ROLE=<n> in the build configuration; do not
 *  edit this file to switch:
 *      Debug_Shell   -> -DUSART1_ROLE=1   bring-up, stages S1..S5
 *      Debug_Modbus  -> -DUSART1_ROLE=2   gateway integration, ships
 *  The default below only keeps a bare compile outside the IDE working; the
 *  normal path always overrides it.
 */

#ifndef INC_BOARD_CONFIG_H_
#define INC_BOARD_CONFIG_H_

#define USART1_ROLE_SHELL		1
#define USART1_ROLE_MODBUS		2

#ifndef USART1_ROLE
#define USART1_ROLE				USART1_ROLE_SHELL
#endif

#if (USART1_ROLE != USART1_ROLE_SHELL) && (USART1_ROLE != USART1_ROLE_MODBUS)
#error "USART1_ROLE must be USART1_ROLE_SHELL(1) or USART1_ROLE_MODBUS(2)"
#endif

/* Modbus slave address and register block base. The gateway forwards absolute
 * 4xxxx addresses unchanged, so this board applies the offset itself. */
#define CSLED_MB_SLAVE_ADDR		(0x55U)
#define CSLED_MB_REG_START		(40401U)

/* Firmware version, read back through status register index 61 */
#define CSLED_FW_VERSION		(0x0100U)	/* v1.00 */

#endif /* INC_BOARD_CONFIG_H_ */
