/*
 * ad5724r.c
 *
 *  Driver for Analog Devices AD5724R quad voltage output DAC.
 *  See ad5724r.h for the 24-bit serial word format.
 *
 *  Created on: 2026年07月07日
 *      Author: ranfa
 */

#include "ad5724r.h"

// ---------- Transfer one 24-bit word (3 bytes) over SPI ----------
static HAL_StatusTypeDef ad5724r_xfer24(ad5724r_t *dev, uint8_t rw,
                                        uint8_t reg, uint8_t addr, uint16_t data)
{
    HAL_StatusTypeDef st = HAL_OK;
    uint8_t tx[3];
    // DB23 = R/W, DB22 = 0, DB21..19 = REG, DB18..16 = A2..A0
    tx[0] = (uint8_t)(((rw & 0x01) << 7) | ((reg & 0x07) << 3) | (addr & 0x07));
    tx[1] = (uint8_t)((data >> 8) & 0xFF);
    tx[2] = (uint8_t)(data & 0xFF);

    if (dev->lock) osMutexAcquire(dev->lock, osWaitForever);

    // SYNC low: start of the write frame
    HAL_GPIO_WritePin(dev->sync_port, dev->sync_pin, GPIO_PIN_RESET);

    st = HAL_SPI_Transmit(dev->hspi, tx, sizeof(tx), HAL_MAX_DELAY);

    // SYNC high: latch the frame on the rising edge
    HAL_GPIO_WritePin(dev->sync_port, dev->sync_pin, GPIO_PIN_SET);

    if (dev->lock) osMutexRelease(dev->lock);
    return st;
}

HAL_StatusTypeDef AD5724R_SetRange(ad5724r_t *dev, ad5724r_channel_t ch,
                                   ad5724r_range_t range)
{
    if (!dev) return HAL_ERROR;
    // Range bits R2..R0 sit in DB2..DB0.
    return ad5724r_xfer24(dev, 0, AD5724R_REG_RANGE, (uint8_t)ch,
                          (uint16_t)(range & 0x07));
}

HAL_StatusTypeDef AD5724R_SetPower(ad5724r_t *dev, uint8_t pu_mask)
{
    if (!dev) return HAL_ERROR;
    // Power control register: address field is don't care, data holds PU bits.
    return ad5724r_xfer24(dev, 0, AD5724R_REG_POWER, 0x0,
                          (uint16_t)(pu_mask & 0x1F));
}

HAL_StatusTypeDef AD5724R_WriteInputOnly(ad5724r_t *dev, ad5724r_channel_t ch,
                                         uint16_t code)
{
    if (!dev) return HAL_ERROR;
    return ad5724r_xfer24(dev, 0, AD5724R_REG_DAC, (uint8_t)ch, code);
}

HAL_StatusTypeDef AD5724R_Update(ad5724r_t *dev)
{
    if (!dev) return HAL_ERROR;
    // Software LOAD: transfer every input register to its DAC output.
    return ad5724r_xfer24(dev, 0, AD5724R_REG_CONTROL, AD5724R_CTRL_LOAD, 0x0000);
}

HAL_StatusTypeDef AD5724R_WriteUpdate(ad5724r_t *dev, ad5724r_channel_t ch,
                                      uint16_t code)
{
    if (!dev) return HAL_ERROR;
    HAL_StatusTypeDef st = AD5724R_WriteInputOnly(dev, ch, code);
    if (st != HAL_OK) return st;
    return AD5724R_Update(dev);
}

HAL_StatusTypeDef AD5724R_BroadcastWriteUpdate(ad5724r_t *dev, uint16_t code)
{
    if (!dev) return HAL_ERROR;
    HAL_StatusTypeDef st = AD5724R_WriteInputOnly(dev, AD5724R_CH_ALL, code);
    if (st != HAL_OK) return st;
    return AD5724R_Update(dev);
}

HAL_StatusTypeDef AD5724R_Init(ad5724r_t *dev,
                               SPI_HandleTypeDef *hspi,
                               GPIO_TypeDef *sync_port, uint16_t sync_pin,
                               ad5724r_range_t range, bool use_internal_ref)
{
    if (!dev || !hspi) return HAL_ERROR;
    dev->hspi = hspi;
    dev->sync_port = sync_port;
    dev->sync_pin = sync_pin;
    dev->range = range;
    dev->use_internal_ref = use_internal_ref;

    // SYNC idle high by default.
    HAL_GPIO_WritePin(sync_port, sync_pin, GPIO_PIN_SET);

    osMutexAttr_t attr = { .name = "ad5724r_lock" };
    dev->lock = osMutexNew(&attr);

    HAL_StatusTypeDef st = HAL_OK;

    // Per the datasheet, the first write after power-up must set the output
    // range on all channels, before powering the channels up.
    st = AD5724R_SetRange(dev, AD5724R_CH_ALL, range);
    if (st != HAL_OK) return st;

    // Power up all four DAC channels (and the internal reference if used).
    uint8_t pu = AD5724R_PU_ALL_DAC;
    if (use_internal_ref) pu |= AD5724R_PU_REF;
    st = AD5724R_SetPower(dev, pu);
    if (st != HAL_OK) return st;

    // Allow the channels/reference to power up (datasheet: 10 us typical).
    HAL_Delay(1);

    return HAL_OK;
}
