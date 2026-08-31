/*
 * debug_shell.c
 *
 *  Created on: 2026年8月31日
 *      Author: ranfa
 *
 *  Bare-metal port of the shell shared by the three RTOS boards. See
 *  debug_shell.h for what changed and why.
 */

#include "debug_shell.h"

/* Keep the translation unit non-empty in the Modbus configuration. */
typedef int shell_translation_unit_not_empty_t;

#if USART1_ROLE == USART1_ROLE_SHELL

#include "bsp_uart.h"

/* --- Structures --- */
typedef struct {
    uint8_t buffer[SHELL_RX_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} RingBuffer_t;

/* --- Static Variables --- */
static LogLevel_t currentLogLevel = LOG_LEVEL_INFO;
static uint32_t logStartTick = 0;
static UART_HandleTypeDef *shellUartHandle = NULL;

/* Buffers */
// Static print buffer replaces stack buffer to prevent stack overflow
static char logPrintBuffer[SHELL_PRINT_BUFFER_SIZE];
static RingBuffer_t rxRingBuffer;
static uint8_t rxSingleByte; // Buffer for HAL_UART_Receive_IT

/* Line editing state. Lives here rather than in Shell_Poll() because the poll
 * function returns between characters. */
static char cmdLine[SHELL_MAX_CMD_LENGTH];
static uint16_t cmdIndex = 0;

/* Command Management */
static DebugCommand_t registeredCommands[SHELL_MAX_COMMANDS];
static size_t commandCount = 0;

/* --- Internal Prototypes --- */
static void HelpCommand(int argc, char *argv[]);
static void ProcessCommand(char *line);
static bool RingBuffer_Read(uint8_t *byte);
static void ShellWrite(const char *s, uint16_t len);

/* --- Built-in Commands --- */
static const DebugCommand_t builtinCommands[] = {
    {"help", "Show all available commands", HelpCommand}
};

/* --- Implementation --- */

/* ---------------------------------------------------------------------------
 * Minimal formatter
 *
 * newlib-nano's vsnprintf costs 5196 B of flash and 312 B of bss on this build:
 * calling it drags in the whole FILE/reentrancy layer (findfp, fflush, fvwrite,
 * __sf) on top of the formatter itself. That does not fit in 16 KB next to the
 * shell. This replacement handles exactly what this board's format strings use
 * and costs a few hundred bytes.
 *
 * Supported: %% %c %s %d %i %u %x %X, flags '-' and '0', decimal field width,
 * and the 'l' length modifier. No precision, no floats (the M0 has no FPU and
 * float formatting is banned board-wide anyway).
 *
 * Deviation from the standard: returns the number of characters ACTUALLY
 * written, not the number that would have been written. Callers here only use
 * it as a buffer index, and this way that index can never run past the end.
 * The result is always NUL-terminated.
 * ------------------------------------------------------------------------- */

#define SHELL_FMT_NUMBUF   12   /* 32-bit decimal with sign, plus slack */

static int shell_vsnprintf(char *out, int size, const char *fmt, va_list ap)
{
    int n = 0;

    if (out == NULL || size <= 0) return 0;

#define PUT(c) do { if (n < size - 1) out[n++] = (char)(c); else goto done; } while (0)

    while (*fmt) {
        if (*fmt != '%') { PUT(*fmt); fmt++; continue; }
        fmt++;                                  /* skip '%' */

        /* flags */
        int leftAlign = 0, zeroPad = 0;
        for (;;) {
            if (*fmt == '-')      { leftAlign = 1; fmt++; }
            else if (*fmt == '0') { zeroPad   = 1; fmt++; }
            else break;
        }

        /* field width */
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt++ - '0');

        /* length modifier: only 'l' changes anything, 'h' is promoted anyway */
        int isLong = 0;
        while (*fmt == 'l' || *fmt == 'h') { if (*fmt == 'l') isLong = 1; fmt++; }

        char conv = *fmt ? *fmt++ : '\0';

        char numbuf[SHELL_FMT_NUMBUF];
        const char *body = numbuf;
        const char *digits = "0123456789abcdef";
        unsigned long uv = 0;
        unsigned base = 10;
        int len = 0, neg = 0, isNum = 1;

        switch (conv) {
        case '\0':
            goto done;
        case '%':
            PUT('%');
            continue;
        case 'c':
            numbuf[0] = (char)va_arg(ap, int);
            len = 1; isNum = 0;
            break;
        case 's': {
            const char *sv = va_arg(ap, const char *);
            if (sv == NULL) sv = "(null)";
            body = sv;
            while (sv[len]) len++;
            isNum = 0;
            break;
        }
        case 'd':
        case 'i': {
            long v = isLong ? va_arg(ap, long) : (long)va_arg(ap, int);
            if (v < 0) { neg = 1; uv = (unsigned long)(-v); }
            else       { uv = (unsigned long)v; }
            break;
        }
        case 'u':
            uv = isLong ? va_arg(ap, unsigned long) : (unsigned long)va_arg(ap, unsigned int);
            break;
        case 'x':
            base = 16;
            uv = isLong ? va_arg(ap, unsigned long) : (unsigned long)va_arg(ap, unsigned int);
            break;
        case 'X':
            base = 16; digits = "0123456789ABCDEF";
            uv = isLong ? va_arg(ap, unsigned long) : (unsigned long)va_arg(ap, unsigned int);
            break;
        default:
            /* unknown conversion: echo it so the mistake is visible */
            PUT('%'); PUT(conv);
            continue;
        }

        if (isNum) {
            char tmp[SHELL_FMT_NUMBUF];
            int t = 0;
            do { tmp[t++] = digits[uv % base]; uv /= base; } while (uv && t < (int)sizeof(tmp));
            if (neg && t < (int)sizeof(tmp)) tmp[t++] = '-';
            for (int i = 0; i < t; i++) numbuf[i] = tmp[t - 1 - i];
            len = t;
        }

        int pad = width - len;
        if (!leftAlign) {
            char padc = (zeroPad && isNum) ? '0' : ' ';
            /* zero padding goes after the sign, not before it */
            if (padc == '0' && neg) { PUT('-'); body = numbuf + 1; len--; }
            while (pad-- > 0) PUT(padc);
        }
        for (int i = 0; i < len; i++) PUT(body[i]);
        if (leftAlign) while (pad-- > 0) PUT(' ');
    }

done:
#undef PUT
    out[n] = '\0';
    return n;
}

static int shell_snprintf(char *out, int size, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = shell_vsnprintf(out, size, fmt, ap);
    va_end(ap);
    return n;
}


/**
  * @brief Initialize debug shell
  */
void Shell_Init(void)
{
    shellUartHandle = &DEBUG_SHELL_UART_HANDLE;
    currentLogLevel = DEFAULT_LOG_LEVEL;
    logStartTick = HAL_GetTick();

    rxRingBuffer.head = 0;
    rxRingBuffer.tail = 0;
    cmdIndex = 0;

    commandCount = 0;
    Shell_RegisterCommands(builtinCommands, sizeof(builtinCommands) / sizeof(builtinCommands[0]));
}

/**
  * @brief Arm reception and print the banner
  */
void Shell_Start(void)
{
    HAL_UART_Receive_IT(shellUartHandle, &rxSingleByte, 1);
    Shell_Print("\r\n>> Shell Ready. Type 'help'.");
    ShellWrite("> ", 2);
}

/**
  * @brief Blocking write helper.
  *
  * Nothing preempts on this board, so a blocking transmit produces a gap-free
  * frame. The DMA-TX machinery the RTOS boards need (see
  * doc/pitfall_notes/Modbus_发送断帧问题溯源与整改总结.md) exists only to survive
  * preemption and is not needed here.
  */
static void ShellWrite(const char *s, uint16_t len)
{
    HAL_UART_Transmit(shellUartHandle, (uint8_t *)s, len, HAL_MAX_DELAY);
}

/**
  * @brief Standard Shell Print
  */
void Shell_Print(const char *format, ...)
{
    if (shellUartHandle == NULL) return;

    va_list args;
    va_start(args, format);
    int pos = shell_vsnprintf(logPrintBuffer, SHELL_PRINT_BUFFER_SIZE, format, args);
    va_end(args);

    /* vsnprintf returns what it *would* have written, so clip before using it
     * as an index. */
    if (pos < 0) return;
    if (pos >= SHELL_PRINT_BUFFER_SIZE) pos = SHELL_PRINT_BUFFER_SIZE - 1;

    /* Add CRLF if missing */
    if (pos > 0 && logPrintBuffer[pos - 1] != '\n') {
        if (pos < SHELL_PRINT_BUFFER_SIZE - 2) {
            logPrintBuffer[pos++] = '\r';
            logPrintBuffer[pos++] = '\n';
        }
    }

    ShellWrite(logPrintBuffer, pos);
}

/**
  * @brief Log Print with Metadata
  */
void Shell_LogPrint(LogLevel_t level, const char *func, int line, const char *format, ...)
{
    if (level < currentLogLevel || shellUartHandle == NULL) return;

    uint32_t ticks = HAL_GetTick() - logStartTick;
    static const char *const levelStr[] = {"DEBUG", "INFO", "WARN", "ERROR", "CRIT"};

    /* Header */
    int pos = shell_snprintf(logPrintBuffer, SHELL_PRINT_BUFFER_SIZE,
                       "[%4lu.%03lu] <%s> %s:%d: ",
                       (unsigned long)(ticks / 1000), (unsigned long)(ticks % 1000),
                       levelStr[level], func, line);
    if (pos < 0) return;
    if (pos >= SHELL_PRINT_BUFFER_SIZE) pos = SHELL_PRINT_BUFFER_SIZE - 1;

    /* User Message */
    va_list args;
    va_start(args, format);
    int n = shell_vsnprintf(logPrintBuffer + pos, SHELL_PRINT_BUFFER_SIZE - pos, format, args);
    va_end(args);
    if (n > 0) {
        pos += n;
        if (pos >= SHELL_PRINT_BUFFER_SIZE) pos = SHELL_PRINT_BUFFER_SIZE - 1;
    }

    /* Newline & Transmit */
    if (pos < SHELL_PRINT_BUFFER_SIZE - 2) {
        logPrintBuffer[pos++] = '\r';
        logPrintBuffer[pos++] = '\n';
    }

    ShellWrite(logPrintBuffer, pos);
}

/**
  * @brief UART RX-complete callback, dispatched from bsp_uart.c
  */
void Shell_UartRecvCallBack(UART_HandleTypeDef *huart)
{
    if (shellUartHandle == NULL || huart->Instance != shellUartHandle->Instance) {
        return;
    }

    /* 1. Push data to Ring Buffer */
    uint16_t next_head = (rxRingBuffer.head + 1) % SHELL_RX_BUFFER_SIZE;
    if (next_head != rxRingBuffer.tail) {
        rxRingBuffer.buffer[rxRingBuffer.head] = rxSingleByte;
        rxRingBuffer.head = next_head;
    }

    /* 2. Restart Reception Immediately */
    HAL_UART_Receive_IT(shellUartHandle, &rxSingleByte, 1);
}

/**
  * @brief Shell UART error handler, dispatched from bsp_uart.c
  *
  * Reception is a single-byte HAL_UART_Receive_IT that is only re-armed from
  * the RX-complete callback, so one line error would otherwise leave the shell
  * deaf for good.
  *
  * NOTE for anyone copying this back to an F4 board: on F0 the CLEAR_*FLAG
  * macros write ICR and each one clears exactly its own flag. On F4 they are
  * all aliases of the same SR+DR read sequence, which clears PE/FE/NE/ORE
  * together. Clearing only PE here would leave a framing or overrun error
  * latched and the freshly re-armed reception would trip again immediately.
  */
void Shell_UartErrorCallBack(UART_HandleTypeDef *huart)
{
    if (shellUartHandle == NULL || huart->Instance != shellUartHandle->Instance) {
        return;
    }
    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_PEF | UART_CLEAR_FEF |
                                 UART_CLEAR_NEF | UART_CLEAR_OREF);
    huart->ErrorCode = HAL_UART_ERROR_NONE;
    HAL_UART_Receive_IT(shellUartHandle, &rxSingleByte, 1);
}

/**
  * @brief Drain the ring buffer and edit the current line. Non-blocking.
  */
void Shell_Poll(void)
{
    uint8_t ch;

    while (RingBuffer_Read(&ch)) {
        /* Handle Backspace */
        if (ch == '\b' || ch == 0x7F) {
            if (cmdIndex > 0) {
                cmdIndex--;
                ShellWrite("\b \b", 3);
            }
        }
        /* Handle Enter */
        else if (ch == '\r' || ch == '\n') {
            ShellWrite("\r\n", 2);

            if (cmdIndex > 0) {
                cmdLine[cmdIndex] = '\0';
                ProcessCommand(cmdLine);
                cmdIndex = 0;
            }

            ShellWrite("> ", 2);
        }
        /* Handle Printable Characters */
        else if (ch >= 32 && ch <= 126) {
            if (cmdIndex < SHELL_MAX_CMD_LENGTH - 1) {
                cmdLine[cmdIndex++] = ch;
                ShellWrite((const char *)&ch, 1);
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

/**
 * @brief Split a command line on spaces, in place.
 *
 * Deliberately not strtok(). newlib's strtok references __assert_func, which
 * pulls in fiprintf and with it the entire stdio stack - fprintf, _vfprintf_r,
 * findfp, fflush, fvwrite, wbuf, wsetup, abort, signal, malloc/sbrk. That chain
 * measured 4.5 KB of flash and 312 B of bss on this build and by itself
 * overflowed the 16 KB part. This replacement is about thirty bytes.
 *
 * Note the caller has already NUL-terminated the line, so the final token needs
 * no terminator of its own.
 */
static int shell_tokenize(char *line, char **argv, int maxArgs)
{
    int argc = 0;
    char *p = line;

    while (argc < maxArgs) {
        while (*p == ' ') p++;                      /* skip separators */
        if (*p == '\0') break;
        argv[argc++] = p;
        while (*p != '\0' && *p != ' ') p++;        /* span the token */
        if (*p == ' ') *p++ = '\0';                 /* terminate it */
    }
    return argc;
}

static void ProcessCommand(char *line)
{
    char *argv[SHELL_MAX_ARGS];
    int argc = shell_tokenize(line, argv, SHELL_MAX_ARGS);

    if (argc == 0) return;

    for (size_t i = 0; i < commandCount; i++) {
        if (strcmp(argv[0], registeredCommands[i].name) == 0) {
            registeredCommands[i].handler(argc, argv);
            return;
        }
    }

    Shell_Print("Unknown command: '%s'", argv[0]);
}

bool Shell_RegisterCommand(const DebugCommand_t *command)
{
    if (command == NULL || command->name == NULL || command->handler == NULL) return false;
    if (commandCount >= SHELL_MAX_COMMANDS) return false;

    /* Check duplicates */
    for (size_t i = 0; i < commandCount; i++) {
        if (strcmp(registeredCommands[i].name, command->name) == 0) return false;
    }

    registeredCommands[commandCount++] = *command;
    return true;
}

bool Shell_RegisterCommands(const DebugCommand_t *commands, size_t count)
{
    if (commands == NULL) return false;
    bool allSuccess = true;
    for (size_t i = 0; i < count; i++) {
        if (!Shell_RegisterCommand(&commands[i])) allSuccess = false;
    }
    return allSuccess;
}

void Shell_SetLogLevel(LogLevel_t level)
{
    currentLogLevel = level;
}

LogLevel_t Shell_GetLogLevel(void)
{
    return currentLogLevel;
}

static void HelpCommand(int argc, char *argv[])
{
    (void)argc; (void)argv;
    Shell_Print(">> Available commands:");
    for (size_t i = 0; i < commandCount; i++) {
        Shell_Print("  %-6s : %s", registeredCommands[i].name, registeredCommands[i].help);
    }
}

#endif /* USART1_ROLE == USART1_ROLE_SHELL */
