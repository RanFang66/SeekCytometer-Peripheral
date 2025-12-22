/*
 * debug_shell.c
 *
 * Created on: 2025年11月26日
 * Author: ranfa
 * Optimized by: Gemini
 */

#include "debug_shell.h"
#include "cmsis_os2.h"
#include "bsp_uart.h"


/* --- Structures --- */
typedef struct {
    uint8_t buffer[SHELL_RX_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} RingBuffer_t;

/* --- Static Variables --- */
static LogLevel_t currentLogLevel = LOG_LEVEL_INFO;
static osMutexId_t shellMutexHandle = NULL;
static osThreadId_t shellTaskHandle = NULL;
static uint32_t logStartTick = 0;
static UART_HandleTypeDef *shellUartHandle = NULL;

/* Buffers */
// Static print buffer replaces stack buffer to prevent stack overflow
static char logPrintBuffer[SHELL_PRINT_BUFFER_SIZE];
static RingBuffer_t rxRingBuffer;
static uint8_t rxSingleByte; // Buffer for HAL_UART_Receive_IT

/* Command Management */
static DebugCommand_t registeredCommands[SHELL_MAX_COMMANDS];
static size_t commandCount = 0;

/* --- Internal Prototypes --- */
static void HelpCommand(int argc, char *argv[]);
static void ProcessCommand(char *cmdLine);
static void CommandTask(void *argument);
static bool RingBuffer_Read(uint8_t *byte);

/* --- Built-in Commands --- */
static const DebugCommand_t builtinCommands[] = {
    {"help", "Show all available commands", HelpCommand}
};

/* --- Implementation --- */

/**
  * @brief Initialize debug shell
  */
void Shell_Init()
{
    shellUartHandle = &DEBUG_SHELL_UART_HANDLE;
    currentLogLevel = DEFAULT_LOG_LEVEL;
    logStartTick = osKernelGetTickCount();

    /* Initialize Ring Buffer */
    rxRingBuffer.head = 0;
    rxRingBuffer.tail = 0;

    /* Initialize Mutex */
    osMutexAttr_t mutex_attr = {
            .name = "shellMutex",
            .attr_bits = osMutexRecursive
    };
    shellMutexHandle = osMutexNew(&mutex_attr);

    /* Register Built-ins */
    commandCount = 0;
    Shell_RegisterCommands(builtinCommands, sizeof(builtinCommands) / sizeof(builtinCommands[0]));

    LOG_INFO("Debug shell initialized. Level: %d", DEFAULT_LOG_LEVEL);
}

/**
  * @brief Start shell debug task and UART Reception
  */
void Shell_StartDebugTask(void)
{
    if(shellTaskHandle != NULL) return;

    osThreadAttr_t attributes = {
        .name = "shellTask",
        .stack_size = SHELL_TASK_STACK_SIZE,
        .priority = osPriorityBelowNormal,
    };

    shellTaskHandle = osThreadNew(CommandTask, NULL, &attributes);

    /* Start Continuous Reception */
    HAL_UART_Receive_IT(shellUartHandle, &rxSingleByte, 1);

    LOG_INFO("Shell task started");
}

/**
  * @brief Standard Shell Print (Thread Safe)
  */
void Shell_Print(const char *format, ...)
{
    if(shellMutexHandle == NULL || shellUartHandle == NULL) return;

    if(osMutexAcquire(shellMutexHandle, 100) != osOK) return;

    va_list args;
    va_start(args, format);
    int pos = vsnprintf(logPrintBuffer, SHELL_PRINT_BUFFER_SIZE, format, args);
    va_end(args);

    /* Safety Clip */
    if (pos >= SHELL_PRINT_BUFFER_SIZE) pos = SHELL_PRINT_BUFFER_SIZE - 1;

    /* Add CRLF if missing */
    if (pos > 0 && logPrintBuffer[pos-1] != '\n') {
        if(pos < SHELL_PRINT_BUFFER_SIZE - 2) {
            logPrintBuffer[pos++] = '\r';
            logPrintBuffer[pos++] = '\n';
            logPrintBuffer[pos] = '\0';
        }
    }

    HAL_UART_Transmit(shellUartHandle, (uint8_t *)logPrintBuffer, pos, HAL_MAX_DELAY);
    osMutexRelease(shellMutexHandle);
}

/**
  * @brief Log Print with Metadata (Thread Safe)
  */
void Shell_LogPrint(LogLevel_t level, const char *func, int line, const char *format, ...)
{
    if(level < currentLogLevel || shellMutexHandle == NULL || shellUartHandle == NULL) return;

    if(osMutexAcquire(shellMutexHandle, 100) != osOK) return;

    uint32_t ticks = osKernelGetTickCount() - logStartTick;
    const char *levelStr[] = {"DEBUG", "INFO", "WARN", "ERROR", "CRIT"};

    /* Header */
    int pos = snprintf(logPrintBuffer, SHELL_PRINT_BUFFER_SIZE,
                       "[%4lu.%03lu] <%s> %s:%d: ",
                       ticks / 1000, ticks % 1000,
                       levelStr[level], func, line);

    /* User Message */
    va_list args;
    va_start(args, format);
    pos += vsnprintf(logPrintBuffer + pos, SHELL_PRINT_BUFFER_SIZE - pos, format, args);
    va_end(args);

    /* Newline & Transmit */
    if(pos < SHELL_PRINT_BUFFER_SIZE - 2) {
        logPrintBuffer[pos++] = '\r';
        logPrintBuffer[pos++] = '\n';
    }

    HAL_UART_Transmit(shellUartHandle, (uint8_t *)logPrintBuffer, pos, HAL_MAX_DELAY);
    osMutexRelease(shellMutexHandle);
}

/**
  * @brief UART ISR Callback (Call this from HAL_UART_RxCpltCallback)
  */
void Shell_UartRecvCallBack(UART_HandleTypeDef *huart)
{
    if(huart->Instance == shellUartHandle->Instance)
    {
        /* 1. Push data to Ring Buffer */
        uint16_t next_head = (rxRingBuffer.head + 1) % SHELL_RX_BUFFER_SIZE;
        if (next_head != rxRingBuffer.tail) {
            rxRingBuffer.buffer[rxRingBuffer.head] = rxSingleByte;
            rxRingBuffer.head = next_head;
        }

        /* 2. Notify Task */
        if(shellTaskHandle != NULL) {
            osThreadFlagsSet(shellTaskHandle, 0x01);
        }

        /* 3. Restart Reception Immediately */
        HAL_UART_Receive_IT(shellUartHandle, &rxSingleByte, 1);
    }
}

/**
  * @brief Main Command Task
  */
static void CommandTask(void *argument)
{
    static char cmdLine[SHELL_MAX_CMD_LENGTH];
    static uint16_t cmdIndex = 0;
    uint8_t ch;

    /* Initial Prompt */
    Shell_Print("\r\n>> Shell Ready. Type 'help'.");
    HAL_UART_Transmit(shellUartHandle, (uint8_t*)"> ", 2, HAL_MAX_DELAY);

    while(1)
    {
        /* Wait for Data Notification */
        osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);

        /* Process all bytes in Ring Buffer */
        while(RingBuffer_Read(&ch))
        {
            /* Handle Backspace */
            if(ch == '\b' || ch == 0x7F)
            {
                if(cmdIndex > 0) {
                    cmdIndex--;
                    /* Handle Echo Manually in Task (Safe) */
                    HAL_UART_Transmit(shellUartHandle, (uint8_t*)"\b \b", 3, 10);
                }
            }
            /* Handle Enter */
            else if(ch == '\r' || ch == '\n')
            {
                HAL_UART_Transmit(shellUartHandle, (uint8_t*)"\r\n", 2, 10);

                if(cmdIndex > 0) {
                    cmdLine[cmdIndex] = '\0';
                    ProcessCommand(cmdLine);
                    cmdIndex = 0;
                }

                HAL_UART_Transmit(shellUartHandle, (uint8_t*)"> ", 2, 10);
            }
            /* Handle Printable Characters */
            else if(ch >= 32 && ch <= 126)
            {
                if(cmdIndex < SHELL_MAX_CMD_LENGTH - 1) {
                    cmdLine[cmdIndex++] = ch;
                    HAL_UART_Transmit(shellUartHandle, &ch, 1, 10);
                }
            }
        }
    }
}

/* --- Helper Functions --- */

static bool RingBuffer_Read(uint8_t *byte)
{
    if (rxRingBuffer.head == rxRingBuffer.tail) {
        return false;
    }
    *byte = rxRingBuffer.buffer[rxRingBuffer.tail];
    rxRingBuffer.tail = (rxRingBuffer.tail + 1) % SHELL_RX_BUFFER_SIZE;
    return true;
}

static void ProcessCommand(char *cmdLine)
{
    char *argv[16];
    int argc = 0;

    // Use strtok_r for thread safety if available, otherwise strtok is okay protected by logic
    char *token = strtok(cmdLine, " ");
    while(token != NULL && argc < 16)
    {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }

    if(argc == 0) return;

    for(size_t i = 0; i < commandCount; i++)
    {
        if(strcmp(argv[0], registeredCommands[i].name) == 0)
        {
            registeredCommands[i].handler(argc, argv);
            return;
        }
    }

    Shell_Print("Unknown command: '%s'", argv[0]);
}

bool Shell_RegisterCommand(const DebugCommand_t *command)
{
    if(command == NULL || command->name == NULL || command->handler == NULL) return false;
    if(commandCount >= SHELL_MAX_COMMANDS) {
        LOG_ERROR("Max commands reached");
        return false;
    }

    /* Check duplicates */
    for(size_t i = 0; i < commandCount; i++) {
        if(strcmp(registeredCommands[i].name, command->name) == 0) return false;
    }

    registeredCommands[commandCount++] = *command;
    return true;
}

bool Shell_RegisterCommands(const DebugCommand_t *commands, size_t count)
{
    if(commands == NULL) return false;
    bool allSuccess = true;
    for(size_t i = 0; i < count; i++) {
        if(!Shell_RegisterCommand(&commands[i])) allSuccess = false;
    }
    return allSuccess;
}

void Shell_SetLogLevel(LogLevel_t level) {
    if(osMutexAcquire(shellMutexHandle, 100) == osOK) {
        currentLogLevel = level;
        osMutexRelease(shellMutexHandle);
    }
}

LogLevel_t Shell_GetLogLevel(void) {
    return currentLogLevel;
}

static void HelpCommand(int argc, char *argv[])
{
    Shell_Print(">> Available commands:");
    for(size_t i = 0; i < commandCount; i++)
    {
        Shell_Print("  %-10s : %s", registeredCommands[i].name, registeredCommands[i].help);
    }
}
