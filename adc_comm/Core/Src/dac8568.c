/*
 * dac8568.c
 *
 *  Created on: 2025年11月26日
 *      Author: ranfa
 */

#include "dac8568.h"


// Transfer voltage to digital value (16bit binary value，0→0V，65535→full range voltage)
static inline uint16_t DAC8568_VoltsToCode(dac8568_t *dev, float volts) {
    if (volts <= 0.0f) return 0;
    float fs = dev->vref;
    if (volts >= fs) return 0xFFFF;
    float code = (volts / fs) * 65535.0f;
    if (code < 0) code = 0;
    if (code > 65535) code = 65535;
    return (uint16_t)(code + 0.5f);
}



// ---------- Transfer 32bit data through SPI ----------
static HAL_StatusTypeDef dac8568_xfer32(dac8568_t *dev, uint32_t word)
{
    HAL_StatusTypeDef st = HAL_OK;
    uint8_t tx[4];
    tx[0] = (uint8_t)((word >> 24) & 0xFF);
    tx[1] = (uint8_t)((word >> 16) & 0xFF);
    tx[2] = (uint8_t)((word >> 8) & 0xFF);
    tx[3] = (uint8_t)(word & 0xFF);

    // Check lock
    if (dev->lock) osMutexAcquire(dev->lock, osWaitForever);

    // SYNC set low, start transfer
    HAL_GPIO_WritePin(dev->sync_port, dev->sync_pin, GPIO_PIN_RESET);

    // Transfer 32bits data (4bytes)
    st = HAL_SPI_Transmit(dev->hspi, tx, sizeof(tx), HAL_MAX_DELAY);

    // SYNC set high
    HAL_GPIO_WritePin(dev->sync_port, dev->sync_pin, GPIO_PIN_SET);

    if (dev->lock) osMutexRelease(dev->lock);
    return st;
}

// SPI data frame: 0 | C3..C0 | A3..A0 | D15..D0 | F3..F0
static inline uint32_t dac8568_make_frame(uint8_t cmd_nibble, uint8_t addr_nibble,
                                          uint16_t data, uint8_t feature_low4)
{
    uint32_t w = 0;
    w |= ((uint32_t)(cmd_nibble & 0x0F)) << 24; // DB27..DB24
    w |= ((uint32_t)(addr_nibble & 0x0F)) << 20; // DB23..DB20
    w |= ((uint32_t)data) << 4;                  // DB19..DB4
    w |= (uint32_t)(feature_low4 & 0x0F);        // DB3..DB0
    // DB31=0; DB30..28 Don't Care=0
    return w;
}

HAL_StatusTypeDef DAC8568_Init(dac8568_t *dev,
                               SPI_HandleTypeDef *hspi,
                               GPIO_TypeDef *sync_port, uint16_t sync_pin,
                               bool enable_internal_ref, float vref_volts)
{
    if (!dev || !hspi) return HAL_ERROR;
    dev->hspi = hspi;
    dev->sync_port = sync_port;
    dev->sync_pin = sync_pin;
    dev->use_internal_ref = enable_internal_ref;
    dev->vref = (vref_volts > 0.0f) ? vref_volts : 2.5f;

    // GPIO: SYNC is set to high by default
    HAL_GPIO_WritePin(sync_port, sync_pin, GPIO_PIN_SET);

    // Create mutex for DAC
    osMutexAttr_t attr = { .name = "dac8568_lock" };
    dev->lock = osMutexNew(&attr);

    HAL_StatusTypeDef st = HAL_OK;

    // Optional: Software reset
    // Software Reset（C=0b0111）：0x07000000
    // Reference：Control Matrix "Software reset (power-on reset)"。
    uint32_t swrst = dac8568_make_frame(DAC8568_SOFTWARE_RESET, 0x0, 0x0000, 0x0);
    st = dac8568_xfer32(dev, swrst);
    if (st != HAL_OK) return st;

    // Optional: Enable internal reference (Flexible mode, or always enable): 0x090A0000
    // If use the external reference voltage, input enable_internal_ref=false and set dev->vref to external reference voltage
    if (enable_internal_ref) {
        st = dac8568_xfer32(dev, 0x090A0000UL);
        if (st != HAL_OK) return st;
        dev->vref = 2.5f; // Internal reference voltage: 2.5V
    }


//    st = dac8568_xfer32(dev, 0x100000FFUL);
    if (st != HAL_OK) return st;

    return HAL_OK;
}

HAL_StatusTypeDef DAC8568_WriteUpdate(dac8568_t *dev,
                                      dac8568_channel_t ch,
                                      uint16_t code)
{
    if (!dev) return HAL_ERROR;
    // Write and update specific channel: C=0b0011，A=channel
    uint32_t frame = dac8568_make_frame(DAC8568_WRITE_UPDATE_CHANNEL, (uint8_t)ch, code, 0x0);
    return dac8568_xfer32(dev, frame);
}

HAL_StatusTypeDef DAC8568_WriteInputOnly(dac8568_t *dev,
                                         dac8568_channel_t ch,
                                         uint16_t code)
{
    if (!dev) return HAL_ERROR;
    // Write value to specific channel but not update: C=0b0000，A=channel
    uint32_t frame = dac8568_make_frame(DAC8568_WRITE_CHANNEL, (uint8_t)ch, code, 0x0);
    return dac8568_xfer32(dev, frame);
}

HAL_StatusTypeDef DAC8568_BroadcastWriteUpdate(dac8568_t *dev,
                                               uint16_t code)
{
    if (!dev) return HAL_ERROR;
    // Broadcast value to all channels and update: C=0b0011，A=0b1111
    uint32_t frame = dac8568_make_frame(DAC8568_WRITE_UPDATE_CHANNEL, DAC8568_CH_ALL, code, 0x0);
    return dac8568_xfer32(dev, frame);
}


HAL_StatusTypeDef DAC8568_UpdateAll(dac8568_t *dev)
{
    if (!dev) return HAL_ERROR;

    // Broadcast Update all: C=0b0001, A=0b1111
    uint32_t frame = dac8568_make_frame(DAC8568_UPDATE_CHANNEL, DAC8568_CH_ALL, 0x0000, 0x0);
    return dac8568_xfer32(dev, frame);
}
