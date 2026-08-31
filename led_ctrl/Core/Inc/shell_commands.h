/*
 * shell_commands.h
 *
 *  Created on: 2026年8月31日
 *      Author: ranfa
 *
 *  Board-specific shell commands, following the cmd -x [args] single-letter
 *  flag convention used by the other boards.
 */

#ifndef INC_SHELL_COMMANDS_H_
#define INC_SHELL_COMMANDS_H_

#include "board_config.h"

#if USART1_ROLE == USART1_ROLE_SHELL
void registerDebugCommands(void);
#else
#define registerDebugCommands()   ((void)0)
#endif

#endif /* INC_SHELL_COMMANDS_H_ */
