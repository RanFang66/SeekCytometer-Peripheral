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
#include "tlc5957.h"
#include "iwdg.h"

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
        /* Read the timer back rather than trusting TLC_GCLK_HZ: that constant
         * goes stale the moment anyone retunes TIM14, and during bring-up
         * that is exactly what happens. */
        {
            uint32_t tclk = HAL_RCC_GetPCLK1Freq();
            uint32_t psc  = TLC_GCLK_TIM.Instance->PSC + 1U;
            uint32_t arr  = TLC_GCLK_TIM.Instance->ARR + 1U;
            uint32_t ccr  = TLC_GCLK_TIM.Instance->CCR1;
            uint32_t gclk = tclk / psc / arr;
            Shell_Print("GCLK=%lu Hz duty=%lu%%  refresh=%lu Hz",
                        (unsigned long)gclk,
                        (unsigned long)(arr ? (ccr * 100U / arr) : 0U),
                        (unsigned long)(gclk / 65536U));
        }
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


/* ---------------------------------------------------------------------------
 * TLC5957 calibration and control
 *
 * "led -n" and "led -w" exist to settle what SLVSCQ4 does not document: the
 * LAT edge count of each command and the bit order of the 768-bit GS latch.
 * See tlc5957.h, the CALIBRATE sections.
 * ------------------------------------------------------------------------- */

static const struct { const char *name; uint8_t n; } tlcCmdTable[] = {
    {"wrtgs",     TLC_CMD_WRTGS},
    {"latgs",     TLC_CMD_LATGS},
    {"wrtfc",     TLC_CMD_WRTFC},
    {"linereset", TLC_CMD_LINERESET},
    {"readfc",    TLC_CMD_READFC},
    {"tmgrst",    TLC_CMD_TMGRST},
    {"fcwrten",   TLC_CMD_FCWRTEN},
};
#define TLC_CMD_TABLE_LEN (sizeof(tlcCmdTable) / sizeof(tlcCmdTable[0]))

static bool argNum(const char *s, long lo, long hi, long *out)
{
    if (!Shell_ParseNum(s, out)) {
        Shell_Print("bad number: '%s'", s);
        return false;
    }
    if (*out < lo || *out > hi) {
        Shell_Print("out of range [%ld..%ld]: %ld", lo, hi, *out);
        return false;
    }
    return true;
}

static void LedCommand(int argc, char *argv[])
{
    long a, b, c, d;

    if (argc < 2 || argv[1][0] != '-') {
        Shell_Print("led -c/-r/-w/-n/-f/-z | diag: -k/-e/-t/-p/-d");
        return;
    }

    switch (argv[1][1]) {

    /* led -c <ch 0..47> <gs 0..65535> : drive one channel */
    case 'c':
        if (argc < 4) { Shell_Print("led -c <ch> <gs>"); return; }
        if (!argNum(argv[2], 0, TLC_CHANNELS - 1, &a)) return;
        if (!argNum(argv[3], 0, 65535, &b)) return;
        TLC5957_Clear();
        TLC5957_SetChannel((uint8_t)a, (uint16_t)b);
        TLC5957_Flush();
        Shell_Print("ch%ld = %ld", a, b);
        break;

    /* led -r <led 0..13> <r> <g> <b> : drive one RGB LED, 0..65535 each */
    case 'r':
        if (argc < 6) { Shell_Print("led -r <led> <r> <g> <b>"); return; }
        if (!argNum(argv[2], 0, TLC_LED_COUNT - 1, &a)) return;
        if (!argNum(argv[3], 0, 65535, &b)) return;
        if (!argNum(argv[4], 0, 65535, &c)) return;
        if (!argNum(argv[5], 0, 65535, &d)) return;
        TLC5957_Clear();
        TLC5957_SetRGB((uint8_t)a, (uint16_t)b, (uint16_t)c, (uint16_t)d);
        TLC5957_Flush();
        Shell_Print("led%ld ch r=%u g=%u b=%u", a,
                    TLC_CH_R((uint8_t)a), TLC_CH_G((uint8_t)a), TLC_CH_B((uint8_t)a));
        break;

    /* led -w <bit 0..767> : walk-a-one.
     * Sets exactly one bit of the 768-bit GS stream and nothing else, so the
     * output that lights identifies what that bit position drives. Bit 0 is
     * the FIRST bit clocked out of the MCU. Sweep 0..767 and record the
     * result; that table IS the answer to CALIBRATE 2 and 3. */
    case 'w': {
        static uint8_t raw[TLC_GS_BYTES];
        if (argc < 3) { Shell_Print("led -w <bitIdx 0..767>"); return; }
        if (!argNum(argv[2], 0, TLC_GS_TOTAL_BITS - 1, &a)) return;
        for (uint16_t i = 0; i < TLC_GS_BYTES; i++) raw[i] = 0;
        raw[a >> 3] = (uint8_t)(0x80U >> (a & 7U));
        TLC5957_FlushRaw(raw);
        /* Unsigned on purpose: a signed / or % here links __divsi3, 460 bytes
         * of libgcc that nothing else on this board needs. The formatter
         * already pays for the unsigned pair. */
        unsigned bit = (unsigned)a;
        Shell_Print("bit %u set (word %u, pos %u)", bit, bit / 48U, bit % 48U);
        Shell_Print("hypothesis: ch%u weight%u",
                    (TLC_GS_LAYOUT == TLC_GS_LAYOUT_BITPLANE)
                        ? (TLC_CHANNELS - 1U - (bit % 48U))
                        : (TLC_CHANNELS - 1U - (bit / 16U)),
                    (TLC_GS_LAYOUT == TLC_GS_LAYOUT_BITPLANE)
                        ? (TLC_GS_BITS - 1U - (bit / 48U))
                        : (TLC_GS_BITS - 1U - (bit % 16U)));
        break;
    }

    /* led -n <cmd> <N> [d0..d5] : send one command with an arbitrary LAT edge
     * count. Use it to confirm each entry of the CALIBRATE 1 table: the value
     * of N that makes the command actually take effect is the right one. */
    case 'n': {
        uint8_t data[6] = {0, 0, 0, 0, 0, 0};
        if (argc < 4) {
            Shell_Print("led -n <cmd> <N> [d0..d5]");
            for (unsigned i = 0; i < TLC_CMD_TABLE_LEN; i++) {
                Shell_Print("  %-9s N=%u", tlcCmdTable[i].name, tlcCmdTable[i].n);
            }
            return;
        }
        if (!argNum(argv[3], 1, TLC_WORD_BITS, &b)) return;
        for (int i = 0; i < 6 && (4 + i) < argc; i++) {
            if (!argNum(argv[4 + i], 0, 255, &c)) return;
            data[i] = (uint8_t)c;
        }
        TLC5957_SendCommand((uint8_t)b, data);
        Shell_Print("sent '%s' with N=%ld", argv[2], b);
        break;
    }

    /* led -f <b0>..<b5> : FCWRTEN then WRTFC with six raw bytes.
     * b0 is clocked out first, i.e. it holds FC bits 47..40. */
    case 'f': {
        uint8_t fc[6] = {0, 0, 0, 0, 0, 0};
        if (argc < 8) { Shell_Print("led -f <b0> <b1> <b2> <b3> <b4> <b5>"); return; }
        for (int i = 0; i < 6; i++) {
            if (!argNum(argv[2 + i], 0, 255, &c)) return;
            fc[i] = (uint8_t)c;
        }
        TLC5957_WriteFC(fc);
        Shell_Print("FC written");
        break;
    }


    /* led -k <nops> : SCLK half-period delay. The board's parasitics turned a
     * 12 MHz GCLK into a near-sine, so the bit-bang rate is suspect too.
     * Sweep this from fast to slow while watching for the first light. */
    case 'k':
        if (argc < 3) { Shell_Print("sclk delay = %u nops", TLC5957_GetSclkDelay()); return; }
        if (!argNum(argv[2], 0, 255, &a)) return;
        TLC5957_SetSclkDelay((uint8_t)a);
        Shell_Print("sclk delay = %ld nops", a);
        break;

    /* led -e <N> : command used for the LAST GS word. 3 = LATGS (datasheet
     * step 3), 7 = LINERESET (step 6, used for the last group of a frame).
     * This board is a single-line display, so every frame is also the last
     * line - which of the two actually starts the display is an open question. */
    case 'e':
        if (argc < 3) { Shell_Print("last word cmd N = %u", TLC5957_GetLastWordCmd()); return; }
        if (!argNum(argv[2], 1, TLC_WORD_BITS, &a)) return;
        TLC5957_SetLastWordCmd((uint8_t)a);
        Shell_Print("last word cmd N = %ld", a);
        break;

    /* led -t <ms> : hammer the current shadow out continuously.
     * If the strip lights only while this runs, the image is not persisting and
     * the answer is XREFRESH / auto display repeat, not the serial protocol. */
    case 't': {
        uint32_t t0, cnt = 0;
        if (argc < 3) { Shell_Print("led -t <ms>"); return; }
        if (!argNum(argv[2], 1, 60000, &a)) return;
        t0 = HAL_GetTick();
        while ((HAL_GetTick() - t0) < (uint32_t)a) {
            TLC5957_Flush();
            cnt++;
            HAL_IWDG_Refresh(&hiwdg);
        }
        Shell_Print("flushed %lu times in %ld ms", (unsigned long)cnt, a);
        break;
    }

    /* led -p <ms> : slow square wave on SCLK/SIN/LAT for scoping. */
    case 'p':
        if (argc < 3) { Shell_Print("led -p <ms>"); return; }
        if (!argNum(argv[2], 1, 60000, &a)) return;
        Shell_Print("wiggling SCLK(1ms) SIN(2ms) LAT(4ms) for %ld ms", a);
        TLC5957_PinWiggle((uint32_t)a);
        Shell_Print("done");
        break;

    /* led -d : dump the tunables */
    case 'd':
        Shell_Print("sclkDelay=%u lastWordCmd=%u layout=%s",
                    TLC5957_GetSclkDelay(), TLC5957_GetLastWordCmd(),
                    (TLC_GS_LAYOUT == TLC_GS_LAYOUT_BITPLANE) ? "BITPLANE" : "CHANNEL");
        Shell_Print("WRTGS=%u LATGS=%u WRTFC=%u LINERESET=%u",
                    TLC_CMD_WRTGS, TLC_CMD_LATGS, TLC_CMD_WRTFC, TLC_CMD_LINERESET);
        break;

    /* led -z : blank everything */
    case 'z':
        TLC5957_Clear();
        TLC5957_Flush();
        Shell_Print("blanked");
        break;

    default:
        Shell_Print("led -c/-r/-w/-n/-f/-z | diag: -k/-e/-t/-p/-d");
        break;
    }
}

static const DebugCommand_t boardCommands[] = {
    {"sys", "System info, use sys -i/-r", SysCommand},
    {"led", "TLC5957, use led -d for options", LedCommand},
};

void registerDebugCommands(void)
{
    Shell_RegisterCommands(boardCommands, sizeof(boardCommands) / sizeof(boardCommands[0]));
    /* The reset flags latch until explicitly cleared; do it once so the next
     * "sys -i" reports the *next* reset rather than this one forever. */
    __HAL_RCC_CLEAR_RESET_FLAGS();
}

#endif /* USART1_ROLE == USART1_ROLE_SHELL */
