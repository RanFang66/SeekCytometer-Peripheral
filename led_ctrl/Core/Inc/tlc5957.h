/*
 * tlc5957.h
 *
 *  Created on: 2026年8月31日
 *      Author: ranfa
 *
 *  Bit-banged driver for the TLC5957 48-channel 16-bit ES-PWM LED driver.
 *
 *  WHY BIT-BANG AND NOT SPI
 *  A command is encoded by how many SCLK rising edges occur while LAT is high,
 *  and every valid count is odd (1/3/5/7/9/11/13). A 48-bit frame is six SPI
 *  bytes, so raising LAT before the last byte can only ever produce eight
 *  edges, which is not a command. Bit-banging is the only way to place LAT on
 *  an exact edge, and at 48 MHz a full 768-bit refresh costs about 150 us -
 *  0.6% of the CPU at 40 fps. See the design doc, section 6.1.
 *
 *  WHAT IS NOT KNOWN YET
 *  reference/tlc5957.pdf is SLVSCQ4, the original 2014 release. It contains
 *  neither the FC register bit map nor the LAT command table; only the command
 *  names appear, in the timing requirements. Everything marked CALIBRATE below
 *  is a hypothesis to be confirmed on the bench at stage S3 using the
 *  "led -n" and "led -w" shell commands. All of it is collected here so that
 *  fixing it is a one-place edit.
 */

#ifndef INC_TLC5957_H_
#define INC_TLC5957_H_

#include <stdint.h>
#include <stdbool.h>

/* ---- Geometry -------------------------------------------------------- */
#define TLC_CHANNELS        48U                     /* OUTR/G/B 0..15 */
#define TLC_GS_BITS         16U                     /* grayscale resolution */
#define TLC_WORD_BITS       48U                     /* common shift register */
#define TLC_GS_TOTAL_BITS   (TLC_CHANNELS * TLC_GS_BITS)    /* 768 */
#define TLC_GS_BYTES        (TLC_GS_TOTAL_BITS / 8U)        /* 96 */
#define TLC_GS_WORDS        (TLC_GS_TOTAL_BITS / TLC_WORD_BITS)  /* 16 */

/* LED15 and LED16 sit in a do-not-populate block on the schematic, so channel
 * indices 14 and 15 of every colour are unused. */
#define TLC_LED_COUNT       14U

/* ---- CALIBRATE 1: LAT command encoding -------------------------------
 * Number of SCLK rising edges for which LAT must stay high. These are the
 * values this device family commonly uses; SLVSCQ4 confirms only the command
 * names. Verify each one with "led -n <cmd> <N>" at S3. */
#define TLC_CMD_WRTGS       1U      /* shift register -> GS latch            */
#define TLC_CMD_LATGS       3U      /* ... and start displaying              */
#define TLC_CMD_WRTFC       5U      /* shift register -> FC latch            */
#define TLC_CMD_LINERESET   7U      /* line counter reset                    */
#define TLC_CMD_READFC      9U      /* FC latch -> shift register            */
#define TLC_CMD_TMGRST      11U     /* display timing reset                  */
#define TLC_CMD_FCWRTEN     13U     /* unlock FC writes                      */

/* ---- CALIBRATE 2: GS latch bit order ---------------------------------
 * How the 768 bits of the GS latch map onto (channel, bit weight). Selected by
 * TLC_GS_LAYOUT and resolved by walking a single 1 through all 768 positions
 * with "led -w <bitIdx>" and recording which output lights. */
#define TLC_GS_LAYOUT_BITPLANE  0   /* one 48-bit word per bit weight        */
#define TLC_GS_LAYOUT_CHANNEL   1   /* 16 consecutive bits per channel       */

#ifndef TLC_GS_LAYOUT
#define TLC_GS_LAYOUT   TLC_GS_LAYOUT_BITPLANE
#endif

/* ---- CALIBRATE 3: colour to channel index ----------------------------
 * Which of the 48 shift-register positions drives OUTRn / OUTGn / OUTBn.
 * Hypothesis: grouped by colour, which is what the three separate CC controls
 * in the datasheet suggest. The physical pin order is interleaved
 * (OUTR0, OUTG0, OUTB0, OUTR1, ...), but pin order need not match shift
 * register order - that is exactly what the S3 walk settles. */
#define TLC_CH_GROUPED_BY_COLOUR    1

#if TLC_CH_GROUPED_BY_COLOUR
#define TLC_CH_R(led)   ((uint8_t)(0U  + (led)))
#define TLC_CH_G(led)   ((uint8_t)(16U + (led)))
#define TLC_CH_B(led)   ((uint8_t)(32U + (led)))
#else
#define TLC_CH_R(led)   ((uint8_t)(3U * (led) + 0U))
#define TLC_CH_G(led)   ((uint8_t)(3U * (led) + 1U))
#define TLC_CH_B(led)   ((uint8_t)(3U * (led) + 2U))
#endif

/* ---- Bring-up knobs --------------------------------------------------
 * The board turned out to have enough parasitic capacitance on these lines
 * that a 12 MHz GCLK on PB1 degenerated into something close to a sine wave.
 * The bit-bang loop free-runs at roughly 4 MHz, which is in the same risk
 * region, so SCLK is slowed down by default and made adjustable at runtime
 * ("led -k"). At 1 MHz a full 768-bit refresh still only costs 770 us.
 *
 * The final GS word is likewise adjustable ("led -e"). LATGS is what the
 * datasheet's step 3 prescribes, but its step 6 uses LINERESET for the last
 * group of a frame, and this board is a single-line display where every frame
 * IS the last line. Which one actually starts the display is an S3 question. */
#define TLC_SCLK_DELAY_DEFAULT      12U     /* NOPs per SCLK half period */
#define TLC_LASTWORD_CMD_DEFAULT    TLC_CMD_LATGS

void TLC5957_SetSclkDelay(uint8_t nops);
uint8_t TLC5957_GetSclkDelay(void);
void TLC5957_SetLastWordCmd(uint8_t latN);
uint8_t TLC5957_GetLastWordCmd(void);

/* Slow square wave on SCLK / SIN / LAT so the lines can be scoped for level
 * and edge quality. Blocks for the requested duration. */
void TLC5957_PinWiggle(uint32_t ms);

/* Hold the three lines at fixed levels. A multimeter at the chip pins is a
 * better instrument than a scope for "is this output actually driving", and it
 * does not depend on catching a 1.5 ms burst. */
void TLC5957_SetPins(bool sclk, bool sin, bool lat);

/* ---- API ------------------------------------------------------------- */

/* Idles the three lines and blanks the GS latch. GCLK must already be running:
 * the grayscale counter only advances while it does. Does NOT write FC - the
 * bit map is unknown, and writing a register blind can set LODVTH, TMGRST or
 * the PWM mode to something confusing. Power-on defaults (BC=4h, CC=100h) are
 * usable until S3 has decoded the register. */
void TLC5957_Init(void);

/* Shadow buffer accessors. Nothing reaches the device until TLC5957_Flush(). */
void TLC5957_SetChannel(uint8_t ch, uint16_t gs);
void TLC5957_SetRGB(uint8_t led, uint16_t r, uint16_t g, uint16_t b);
void TLC5957_Clear(void);
uint16_t TLC5957_GetChannel(uint8_t ch);

/* Serialize the shadow buffer: 16 x (48 bits + WRTGS), the last one LATGS. */
void TLC5957_Flush(void);

/* Calibration back doors, used by the S3 shell commands. */
void TLC5957_FlushRaw(const uint8_t raw[TLC_GS_BYTES]);
void TLC5957_SendCommand(uint8_t latN, const uint8_t data[6]);
void TLC5957_WriteFC(const uint8_t fc[6]);

/* Position of (channel, weight) within the 768-bit stream, counted from the
 * first bit clocked out. Exposed so the walk-a-one command can print the
 * expected inverse mapping next to what was actually observed. */
uint16_t TLC5957_GsBitIndex(uint8_t ch, uint8_t weight);

#endif /* INC_TLC5957_H_ */
