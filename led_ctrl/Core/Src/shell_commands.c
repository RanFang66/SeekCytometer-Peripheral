/*
 * shell_commands.c
 *
 *  Created on: 2026年8月31日
 *      Author: ranfa
 */

#include "shell_commands.h"

/* Keep the translation unit non-empty in the Modbus configuration. */
typedef int shell_commands_translation_unit_not_empty_t;

#if USART1_ROLE == USART1_ROLE_SHELL

#include "debug_shell.h"
#include "bsp_tim.h"

/* Linker-provided RAM layout symbols, see STM32F030F4PX_FLASH.ld */
extern uint32_t _sdata, _edata, _sbss, _ebss, _estack;

/**
 * @brief Decode and clear the reset cause.
 *
 * Worth having on this board: the IWDG is armed from MX_IWDG_Init() and there
 * is no other way to tell a watchdog reset from a power cycle. The flags latch
 * until cleared, so read them once at startup.
 */
static const char *resetCause(void)
{
    const char *s = "UNKNOWN";
    if      (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) s = "IWDG";
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))  s = "SOFT";
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST))  s = "PIN";
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST))  s = "POR";
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST)) s = "LPWR";
    return s;
}

static void SysCommand(int argc, char *argv[])
{
    if (argc < 2 || argv[1][0] != '-') {
        Shell_Print("Usage: sys -i");
        return;
    }

    switch (argv[1][1]) {
    case 'i': {
        uint32_t up = HAL_GetTick() / 1000U;
        uint32_t staticRam = (uint32_t)&_ebss - (uint32_t)&_sdata;
        uint32_t stackRsv  = (uint32_t)&_estack - (uint32_t)&_ebss;

        Shell_Print("FW v%u.%02u  role=%s  reset=%s",
                    (unsigned)(CSLED_FW_VERSION >> 8),
                    (unsigned)(CSLED_FW_VERSION & 0xFF),
                    (USART1_ROLE == USART1_ROLE_SHELL) ? "SHELL" : "MODBUS",
                    resetCause());
        Shell_Print("SYSCLK=%lu Hz  uptime=%lus",
                    (unsigned long)SystemCoreClock, (unsigned long)up);
        Shell_Print("GCLK=%lu Hz  refresh=%lu Hz",
                    (unsigned long)TLC_GCLK_HZ, (unsigned long)TLC_REFRESH_HZ);
        Shell_Print("RAM static=%luB  stack free>=%luB",
                    (unsigned long)staticRam, (unsigned long)stackRsv);
        break;
    }
    case 'r':
        Shell_Print("Resetting...");
        HAL_Delay(20);              /* let the line drain before we go */
        NVIC_SystemReset();
        break;
    default:
        Shell_Print("Usage: sys -i/-r");
        break;
    }
}

static const DebugCommand_t boardCommands[] = {
    {"sys", "System info, use sys -i/-r", SysCommand},
};

void registerDebugCommands(void)
{
    Shell_RegisterCommands(boardCommands, sizeof(boardCommands) / sizeof(boardCommands[0]));
    /* The reset flags latch until explicitly cleared; do it once so the next
     * "sys -i" reports the *next* reset rather than this one forever. */
    __HAL_RCC_CLEAR_RESET_FLAGS();
}

#endif /* USART1_ROLE == USART1_ROLE_SHELL */
