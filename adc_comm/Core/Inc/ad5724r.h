/*
 * ad5724r.h
 *
 *  Driver for Analog Devices AD5724R / AD5734R / AD5754R quad, serial input,
 *  unipolar/bipolar voltage output DAC (this project uses the AD5724R, but the
 *  16-bit DAC register format of the AD5754R is used to keep full resolution;
 *  on the 12-bit AD5724R only the upper 12 bits are significant).
 *
 *  The device is controlled through a 24-bit SPI word (MSB first):
 *      DB23      : R/W (0 = write, 1 = read)
 *      DB22      : Zero (must be 0)
 *      DB21..19  : REG2..REG0  (register select)
 *      DB18..16  : A2..A0      (DAC channel address)
 *      DB15..0   : 16-bit data
 *
 *  Created on: 2026年07月07日
 *      Author: ranfa
 */

#ifndef INC_AD5724R_H_
#define INC_AD5724R_H_

#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Register select values (REG2..REG0)
#define AD5724R_REG_DAC        0x0    // DAC (input) register
#define AD5724R_REG_RANGE      0x1    // Output range select register
#define AD5724R_REG_POWER      0x2    // Power control register
#define AD5724R_REG_CONTROL    0x3    // Control register

// Control register sub-functions (placed in the A2..A0 address field)
#define AD5724R_CTRL_NOP       0x0    // No operation
#define AD5724R_CTRL_FUNCTION  0x1    // Configure TSD/clamp/CLR-select/SDO
#define AD5724R_CTRL_CLEAR     0x4    // Clear all DAC registers to CLR code
#define AD5724R_CTRL_LOAD      0x5    // Load: transfer input regs -> DAC outputs

// Power control register bit masks (DB0..DB4)
#define AD5724R_PU_A           (1u << 0)
#define AD5724R_PU_B           (1u << 1)
#define AD5724R_PU_C           (1u << 2)
#define AD5724R_PU_D           (1u << 3)
#define AD5724R_PU_REF         (1u << 4)
#define AD5724R_PU_ALL_DAC     (AD5724R_PU_A | AD5724R_PU_B | AD5724R_PU_C | AD5724R_PU_D)

// DAC channel address (A2..A0)
typedef enum {
    AD5724R_CH_A   = 0x0,
    AD5724R_CH_B   = 0x1,
    AD5724R_CH_C   = 0x2,
    AD5724R_CH_D   = 0x3,
    AD5724R_CH_ALL = 0x4,   // addresses all four DACs
} ad5724r_channel_t;

// Output range codes (R2..R0 in the output range select register)
typedef enum {
    AD5724R_RANGE_5V      = 0x0,   // 0 V .. +5 V   (unipolar, gain 2)
    AD5724R_RANGE_10V     = 0x1,   // 0 V .. +10 V  (unipolar, gain 4)
    AD5724R_RANGE_10V8    = 0x2,   // 0 V .. +10.8 V(unipolar, gain 4.32)
    AD5724R_RANGE_BI_5V   = 0x3,   // -5 V .. +5 V  (bipolar,  gain 4)
    AD5724R_RANGE_BI_10V  = 0x4,   // -10 V .. +10 V(bipolar,  gain 8)
    AD5724R_RANGE_BI_10V8 = 0x5,   // -10.8 V..+10.8V(bipolar, gain 8.64)
} ad5724r_range_t;

typedef struct {
    SPI_HandleTypeDef *hspi;       // SPI handle (shared bus)
    GPIO_TypeDef      *sync_port;  // SYNC (chip select) port
    uint16_t           sync_pin;   // SYNC (chip select) pin
    osMutexId_t        lock;       // Mutex for thread safety
    ad5724r_range_t    range;      // Configured output range
    bool               use_internal_ref; // Use on-chip 2.5 V reference
} ad5724r_t;

/*
 * Initialize an AD5724R device.
 *  - Sets the output range on all four channels.
 *  - Powers up all four DAC channels (and the internal reference when
 *    use_internal_ref is true).
 *
 * NOTE on LDAC: this driver updates outputs through the software LOAD command
 * (control register). It therefore works whether the LDAC pin is tied high or
 * low. If LDAC is tied low the outputs also update on each SYNC rising edge, so
 * "WriteInputOnly" behaves like an immediate write; the trailing LOAD in the
 * batch helpers is then a harmless no-op.
 */
HAL_StatusTypeDef AD5724R_Init(ad5724r_t *dev,
                               SPI_HandleTypeDef *hspi,
                               GPIO_TypeDef *sync_port, uint16_t sync_pin,
                               ad5724r_range_t range, bool use_internal_ref);

// Configure the output range of a channel (or AD5724R_CH_ALL).
HAL_StatusTypeDef AD5724R_SetRange(ad5724r_t *dev, ad5724r_channel_t ch,
                                   ad5724r_range_t range);

// Power control: pu_mask is an OR of AD5724R_PU_x bits.
HAL_StatusTypeDef AD5724R_SetPower(ad5724r_t *dev, uint8_t pu_mask);

// Write a channel's input register without updating the output.
HAL_StatusTypeDef AD5724R_WriteInputOnly(ad5724r_t *dev, ad5724r_channel_t ch,
                                         uint16_t code);

// Transfer all input registers to the DAC outputs (software LOAD).
HAL_StatusTypeDef AD5724R_Update(ad5724r_t *dev);

// Write a channel and update its output immediately.
HAL_StatusTypeDef AD5724R_WriteUpdate(ad5724r_t *dev, ad5724r_channel_t ch,
                                      uint16_t code);

// Write the same code to all four channels and update the outputs.
HAL_StatusTypeDef AD5724R_BroadcastWriteUpdate(ad5724r_t *dev, uint16_t code);

#ifdef __cplusplus
}
#endif

#endif /* INC_AD5724R_H_ */
