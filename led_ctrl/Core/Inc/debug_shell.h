/*
 * debug_shell.h
 *
 *  Created on: 2026年8月31日
 *      Author: ranfa
 *
 *  Bare-metal port of the shell shared by adc_comm / motor_ctrl / mfc_ctrl.
 *  Differences from those copies, all forced by this board:
 *    - no CMSIS-OS: no mutex (nothing preempts), no task, no thread flags.
 *      Shell_StartDebugTask() becomes Shell_Start() plus Shell_Poll() in the
 *      superloop.
 *    - buffers cut down for 4 KB of RAM, command table cut down for 16 KB of
 *      flash.
 *    - the whole thing compiles out in the Debug_Modbus configuration, where
 *      USART1 belongs to the Modbus slave instead. The logging macros degrade
 *      to no-ops there so shared modules can call LOG_*() unconditionally.
 */

#ifndef INC_DEBUG_SHELL_H_
#define INC_DEBUG_SHELL_H_

/* <stdio.h> deliberately not included: see the minimal formatter in debug_shell.c */
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include "stm32f0xx_hal.h"
#include "board_config.h"

#if USART1_ROLE == USART1_ROLE_SHELL

/* --- Configuration ---
 * Sized for this part, not copied from the F4 boards: 96 B of print buffer is
 * enough for every line this board emits, and 8 command slots cover
 * help/led/lock/sys with room to spare. */
#define SHELL_PRINT_BUFFER_SIZE  96     // Max length of a log line
#define SHELL_RX_BUFFER_SIZE     64     // Size of the internal Ring Buffer
#define SHELL_MAX_COMMANDS       8      // Max number of registered commands
#define SHELL_MAX_CMD_LENGTH     64     // Max length of a user command line
#define SHELL_MAX_ARGS           8      // Max tokens per command line

/* Log Level Type Definition */
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_CRITICAL,
    LOG_LEVEL_NONE    // Close all logs
} LogLevel_t;

#define DEFAULT_LOG_LEVEL LOG_LEVEL_INFO

/* Command Handler Typedef */
typedef void (*DebugCommandHandler_t)(int argc, char *argv[]);

/* Command Structure */
typedef struct {
    const char *name;
    const char *help;
    DebugCommandHandler_t handler;
} DebugCommand_t;

/* Public API */
void Shell_Init(void);
void Shell_Start(void);     /* arms RX, prints the banner */
void Shell_Poll(void);      /* drains the ring buffer; call from the superloop */

/* Logger API */
void Shell_Print(const char *format, ...);
void Shell_LogPrint(LogLevel_t level, const char *func, int line, const char *format, ...);
void Shell_SetLogLevel(LogLevel_t level);
LogLevel_t Shell_GetLogLevel(void);

/* Number parsing. Deliberately not strtol()/atoi(): see the note on strtok in
 * debug_shell.c - libc entry points that can reach __assert_func drag the whole
 * stdio stack in behind them. Accepts decimal, 0x hex and a leading '-'.
 * Returns false on a malformed string so callers can reject it. */
bool Shell_ParseNum(const char *s, long *out);

/* Command Registration */
bool Shell_RegisterCommand(const DebugCommand_t *cmd);
bool Shell_RegisterCommands(const DebugCommand_t *cmds, size_t count);

/* Interrupt Callbacks - dispatched from bsp_uart.c */
void Shell_UartRecvCallBack(UART_HandleTypeDef *huart);
void Shell_UartErrorCallBack(UART_HandleTypeDef *huart);

/* Macros for fast logging */
#define LOG_DEBUG(...)    Shell_LogPrint(LOG_LEVEL_DEBUG, __func__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)     Shell_LogPrint(LOG_LEVEL_INFO, __func__, __LINE__, __VA_ARGS__)
#define LOG_WARNING(...)  Shell_LogPrint(LOG_LEVEL_WARNING, __func__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...)    Shell_LogPrint(LOG_LEVEL_ERROR, __func__, __LINE__, __VA_ARGS__)
#define LOG_CRITICAL(...) Shell_LogPrint(LOG_LEVEL_CRITICAL, __func__, __LINE__, __VA_ARGS__)

#else /* USART1_ROLE == USART1_ROLE_MODBUS */

/* No shell in this configuration. The no-ops let shared modules keep their
 * LOG_*() calls without #if clutter; the format strings are discarded by the
 * compiler, so they cost no flash. */
#define Shell_Print(...)        ((void)0)
#define Shell_LogPrint(...)     ((void)0)
#define LOG_DEBUG(...)          ((void)0)
#define LOG_INFO(...)           ((void)0)
#define LOG_WARNING(...)        ((void)0)
#define LOG_ERROR(...)          ((void)0)
#define LOG_CRITICAL(...)       ((void)0)

#endif /* USART1_ROLE */

#endif /* INC_DEBUG_SHELL_H_ */
