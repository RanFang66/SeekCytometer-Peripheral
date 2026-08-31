/*
 * board_config.h
 *
 *  Created on: 2026年8月31日
 *      Author: ranfa
 *
 *  Board-level compile-time switches for CS_LED_Ctrl V1.0.
 *
 *  STM32F030F4P6 只有 USART1 一个串口（PA9/PA10 那组复用脚 PA9 悬空、PA10 已作
 *  AUX_IN，而且即使接出来也还是同一个 USART1 外设），物理上不存在第二条串行通道。
 *  所以调试 shell 和 Modbus 从站只能二选一 —— 见
 *  doc/design/CS_LED_Lock_Ctrl_详细设计.md §5.2。
 *
 *  角色由 build configuration 里的 -DUSART1_ROLE=<n> 决定，不要改这个文件：
 *      Debug_Shell   -> -DUSART1_ROLE=1   前期调试，S1~S5 阶段
 *      Debug_Modbus  -> -DUSART1_ROLE=2   与 adc_comm 联调，S6 之后即交付构型
 *  下面的默认值只是保证脱离 IDE 直接编译时也能过，正常路径上一定被 -D 覆盖。
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

/* 本板 Modbus 从站地址与寄存器块基址（网关转发绝对地址，由本板自己做偏移） */
#define CSLED_MB_SLAVE_ADDR		(0x55U)
#define CSLED_MB_REG_START		(40401U)

/* 固件版本，回读寄存器 idx 61 */
#define CSLED_FW_VERSION		(0x0100U)	/* v1.00 */

#endif /* INC_BOARD_CONFIG_H_ */
