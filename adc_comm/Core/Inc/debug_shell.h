/*
 * debug_shell.h
 *
 * Created on: 2025年11月26日
 * Author: ranfa
 * Optimized by: Gemini
 */

#ifndef INC_DEBUG_SHELL_H_
#define INC_DEBUG_SHELL_H_

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"  // Ensure this matches your specific MCU family

/* --- Configuration --- */
#define SHELL_PRINT_BUFFER_SIZE  512    // Max length of a log line
#define SHELL_RX_BUFFER_SIZE     128    // Size of the internal Ring Buffer
#define SHELL_MAX_COMMANDS       32     // Max number of registered commands
#define SHELL_MAX_CMD_LENGTH     128    // Max length of a user command line
#define SHELL_TASK_STACK_SIZE    2048   // Stack size for the shell task

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
void Shell_Init();
void Shell_StartDebugTask(void);

/* Logger API */
void Shell_Print(const char *format, ...);
void Shell_LogPrint(LogLevel_t level, const char *func, int line, const char *format, ...);
void Shell_SetLogLevel(LogLevel_t level);
LogLevel_t Shell_GetLogLevel(void);

/* Command Registration */
bool Shell_RegisterCommand(const DebugCommand_t *cmd);
bool Shell_RegisterCommands(const DebugCommand_t *cmds, size_t count);

/* Interrupt Callback - Call this from HAL_UART_RxCpltCallback */
void Shell_UartRecvCallBack(UART_HandleTypeDef *huart);

/* Macros for fast logging */
#define LOG_DEBUG(...)    Shell_LogPrint(LOG_LEVEL_DEBUG, __func__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)     Shell_LogPrint(LOG_LEVEL_INFO, __func__, __LINE__, __VA_ARGS__)
#define LOG_WARNING(...)  Shell_LogPrint(LOG_LEVEL_WARNING, __func__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...)    Shell_LogPrint(LOG_LEVEL_ERROR, __func__, __LINE__, __VA_ARGS__)
#define LOG_CRITICAL(...) Shell_LogPrint(LOG_LEVEL_CRITICAL, __func__, __LINE__, __VA_ARGS__)

#endif /* INC_DEBUG_SHELL_H_ */
