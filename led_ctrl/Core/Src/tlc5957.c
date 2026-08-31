/*
 * tlc5957.c
 *
 *  Created on: 2026年8月31日
 *      Author: ranfa
 */

#include "tlc5957.h"
#include "bsp_gpio.h"
#include "bsp_tim.h"

/* Direct register access rather than HAL_GPIO_WritePin: this is the innermost
 * loop of the whole board and the HAL call would triple the edge cost. BSRR
 * sets, BRR clears - both are single stores with no read-modify-write, so no
 * interrupt can corrupt a neighbouring pin. */
#define SCLK_HI()   (TLC_SCLK_PORT->BSRR = TLC_SCLK_PIN)
#define SCLK_LO()   (TLC_SCLK_PORT->BRR  = TLC_SCLK_PIN)
#define LAT_HI()    (TLC_LAT_PORT->BSRR  = TLC_LAT_PIN)
#define LAT_LO()    (TLC_LAT_PORT->BRR   = TLC_LAT_PIN)
#define SIN_HI()    (TLC_SIN_PORT->BSRR  = TLC_SIN_PIN)
#define SIN_LO()    (TLC_SIN_PORT->BRR   = TLC_SIN_PIN)

/* tSU2: LAT falling to the next SCLK rising edge. 20 ns for WRTGS/WRTFC/TMGRST
 * but 80 ns for LATGS/READFC/LINERESET. 80 ns is 3.84 cycles at 48 MHz, which a
 * tight bit-bang loop would violate, so pad unconditionally with margin.
 * This is the ONLY timing in the whole sequence the loop can get wrong; every
 * other constraint in the datasheet is a minimum of 2-10 ns, and a single
 * Cortex-M0 instruction already takes 20.8 ns. */
#define TLC_TSU2_DELAY()    do { __NOP(); __NOP(); __NOP(); __NOP(); \
                                 __NOP(); __NOP(); } while (0)

/* Channel-indexed shadow. 48 x 2 = 96 bytes. */
static uint16_t gsShadow[TLC_CHANNELS];

static uint8_t sclkDelay    = TLC_SCLK_DELAY_DEFAULT;
static uint8_t lastWordCmd  = TLC_LASTWORD_CMD_DEFAULT;

/* Deliberately a counted NOP loop rather than a DWT/timer delay: it has to work
 * with interrupts on, it must not depend on any peripheral, and the exact
 * frequency does not matter - only that it is slow enough for the board. */
static inline void sclk_delay(void)
{
    for (uint8_t k = sclkDelay; k != 0U; k--) {
        __NOP();
    }
}

void TLC5957_SetSclkDelay(uint8_t nops)  { sclkDelay = nops; }
uint8_t TLC5957_GetSclkDelay(void)       { return sclkDelay; }
void TLC5957_SetLastWordCmd(uint8_t n)   { lastWordCmd = n; }
uint8_t TLC5957_GetLastWordCmd(void)     { return lastWordCmd; }

/* ------------------------------------------------------------------------ */

uint16_t TLC5957_GsBitIndex(uint8_t ch, uint8_t weight)
{
#if TLC_GS_LAYOUT == TLC_GS_LAYOUT_BITPLANE
    /* One 48-bit word per bit weight, most significant weight clocked first;
     * within a word the highest channel index goes first. */
    return (uint16_t)((TLC_GS_BITS - 1U - weight) * TLC_WORD_BITS
                      + (TLC_CHANNELS - 1U - ch));
#else
    /* 16 consecutive bits per channel, highest channel first, MSB first. */
    return (uint16_t)((TLC_CHANNELS - 1U - ch) * TLC_GS_BITS
                      + (TLC_GS_BITS - 1U - weight));
#endif
}

/**
 * @brief Clock out 48 bits, holding LAT high across the final latN rising edges.
 *
 * data[0] bit 7 is sent first. The device shifts SIN into the shift register
 * LSB on each SCLK rising edge and moves the contents towards the MSB, so the
 * first bit sent ends up as bit 47 - i.e. plain MSB-first ordering.
 *
 * Interrupts stay enabled on purpose. The command encoding counts edges, not
 * time; an interrupt can only stretch the SCLK low phase and the LAT high
 * phase, and the datasheet puts a minimum on both and a maximum on neither.
 * Do not wrap this in a critical section - it would delay UART service for no
 * benefit.
 */
static void tlc_write48(const uint8_t data[6], uint8_t latN)
{
    for (int i = (int)TLC_WORD_BITS - 1; i >= 0; i--) {
        uint8_t bitPos = (uint8_t)((TLC_WORD_BITS - 1 - i));
        if (data[bitPos >> 3] & (uint8_t)(0x80U >> (bitPos & 7U))) {
            SIN_HI();
        } else {
            SIN_LO();
        }

        /* Raise LAT just before the rising edge that starts the count. */
        if (i == (int)latN - 1) {
            LAT_HI();
        }

        sclk_delay();
        SCLK_HI();      /* rising edge: SIN sampled, command edge counted */
        sclk_delay();
        SCLK_LO();
    }

    /* "LAT Signal needs to include falling edge of SCLK" (SLVSCQ4 Figure 1):
     * SCLK is already low here, so lowering LAT now satisfies it. */
    LAT_LO();
    TLC_TSU2_DELAY();
}

void TLC5957_SendCommand(uint8_t latN, const uint8_t data[6])
{
    tlc_write48(data, latN);
}

/* ------------------------------------------------------------------------ */

void TLC5957_Init(void)
{
    SCLK_LO();
    SIN_LO();
    LAT_LO();

    TLC5957_Clear();
    TLC5957_Flush();
}

void TLC5957_Clear(void)
{
    for (uint8_t i = 0; i < TLC_CHANNELS; i++) {
        gsShadow[i] = 0;
    }
}

void TLC5957_SetChannel(uint8_t ch, uint16_t gs)
{
    if (ch < TLC_CHANNELS) {
        gsShadow[ch] = gs;
    }
}

uint16_t TLC5957_GetChannel(uint8_t ch)
{
    return (ch < TLC_CHANNELS) ? gsShadow[ch] : 0U;
}

void TLC5957_SetRGB(uint8_t led, uint16_t r, uint16_t g, uint16_t b)
{
    if (led >= TLC_LED_COUNT) {
        return;
    }
    gsShadow[TLC_CH_R(led)] = r;
    gsShadow[TLC_CH_G(led)] = g;
    gsShadow[TLC_CH_B(led)] = b;
}

/**
 * @brief Send a fully formed 768-bit GS image.
 *
 * 16 words of 48 bits. Every word but the last is terminated with WRTGS; the
 * last one uses LATGS, which both stores it and starts displaying the whole
 * latch.
 */
void TLC5957_FlushRaw(const uint8_t raw[TLC_GS_BYTES])
{
    for (uint8_t w = 0; w < TLC_GS_WORDS; w++) {
        const uint8_t *word = &raw[w * (TLC_WORD_BITS / 8U)];
        tlc_write48(word, (w == TLC_GS_WORDS - 1U) ? lastWordCmd
                                                   : TLC_CMD_WRTGS);
    }
}

void TLC5957_Flush(void)
{
    uint8_t raw[TLC_GS_BYTES];

    for (uint16_t i = 0; i < TLC_GS_BYTES; i++) {
        raw[i] = 0;
    }

    for (uint8_t ch = 0; ch < TLC_CHANNELS; ch++) {
        uint16_t v = gsShadow[ch];
        if (v == 0U) {
            continue;                       /* the common case, skip 16 shifts */
        }
        for (uint8_t weight = 0; weight < TLC_GS_BITS; weight++) {
            if (v & (uint16_t)(1U << weight)) {
                uint16_t idx = TLC5957_GsBitIndex(ch, weight);
                raw[idx >> 3] |= (uint8_t)(0x80U >> (idx & 7U));
            }
        }
    }

    TLC5957_FlushRaw(raw);
}

/**
 * @brief Write the 48-bit function control register.
 *
 * Not called from TLC5957_Init(): until S3 has decoded the bit map, writing
 * this register blind risks flipping LODVTH, the PWM mode select or TMGRST and
 * producing symptoms that look like a driver bug. Reach it through "led -f"
 * during calibration.
 */
void TLC5957_WriteFC(const uint8_t fc[6])
{
    static const uint8_t zeros[6] = {0, 0, 0, 0, 0, 0};

    tlc_write48(zeros, TLC_CMD_FCWRTEN);    /* unlock */
    tlc_write48(fc, TLC_CMD_WRTFC);         /* write  */
}

/**
 * @brief Drive the three lines with a slow square wave so they can be scoped.
 *
 * SCLK and SIN toggle every millisecond, LAT every four, so all three are
 * distinguishable on one trace. This answers the most basic question a dark
 * strip raises: do the pins actually move, and do they reach valid levels at
 * the far end of the trace? VIH is 0.7 x VCC = 2.31 V here, VCC being 3.3 V.
 */
void TLC5957_PinWiggle(uint32_t ms)
{
    uint32_t start = HAL_GetTick();
    uint32_t n = 0;

    while ((HAL_GetTick() - start) < ms) {
        if (n & 1U) { SCLK_HI(); } else { SCLK_LO(); }
        if (n & 2U) { SIN_HI();  } else { SIN_LO();  }
        if (n & 4U) { LAT_HI();  } else { LAT_LO();  }
        n++;
        HAL_Delay(1);
    }
    SCLK_LO();
    SIN_LO();
    LAT_LO();
}
