/*
 * hsc_spi.c
 *
 *  Created on: Oct 24, 2025
 *      Author: ranfa
 */


#include "hsc_spi.h"
#include <string.h>
#include "gpio.h"
#include "spi.h"
#include <stdlib.h>
#include "debug_shell.h"
#include "hsc_conv.h"


static SPI_HandleTypeDef *const hscSPI = &hspi3;
static hsc_bus_t hscInstances;
static hsc_bus_t * const bus = &hscInstances;
static GPIO_TypeDef * const cs_ports[] = {GPIOC, GPIOC, GPIOA, GPIOB, GPIOB, GPIOA};
static const uint16_t cs_pins[] = {GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_8, GPIO_PIN_3, GPIO_PIN_4, GPIO_PIN_1};

static inline void cs_low(GPIO_TypeDef *port, uint16_t pin)  { HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET); }
static inline void cs_high(GPIO_TypeDef *port, uint16_t pin) { HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET); }


// ------- 帧解析 -------
// 常见 HSC SPI 格式：
// Byte0: [S1 S0 P13 P12 P11 P10 P9 P8]
// Byte1: [P7 P6 P5 P4 P3 P2 P1 P0]
// Byte2: [T10 T9 T8 T7 T6 T5 T4 T3]
// Byte3: [T2 T1 T0 x x x x x]
// 压力 14-bit：0..16383；温度 11-bit：0..2047
static void hsc_parse_frame(const uint8_t rx[HSC_FRAME_LEN],
                            uint16_t *p_counts,
                            uint16_t *t_counts,
                            hsc_status_t *status_out)
{
    uint8_t s1s0 = (rx[0] >> 6) & 0x03;
    uint16_t p14 = ((uint16_t)(rx[0] & 0x3F) << 8) | rx[1];
    uint16_t t11 = ((uint16_t)rx[2] << 3) | (rx[3] >> 5);

    if (p_counts)   *p_counts   = (p14 & 0x3FFF);
    if (t_counts)   *t_counts   = (t11 & 0x07FF);
    if (status_out) *status_out = (hsc_status_t)s1s0;
}

// ------- 低层一帧收发（带互斥） -------
static HAL_StatusTypeDef hsc_xfer_locked(hsc_bus_t *bus, uint8_t idx, uint8_t rx[HSC_FRAME_LEN])
{
    static uint8_t tx_dummy[HSC_FRAME_LEN] = {0,0,0,0};
    HAL_StatusTypeDef st;

    osMutexAcquire(bus->bus_lock, osWaitForever);

    cs_low(bus->cs_port[idx], bus->cs_pin[idx]);
    st = HAL_SPI_TransmitReceive(bus->hspi, (uint8_t*)tx_dummy, rx, HSC_FRAME_LEN, HAL_MAX_DELAY);
    cs_high(bus->cs_port[idx], bus->cs_pin[idx]);

    osMutexRelease(bus->bus_lock);
    return st;
}

// ------- 读一帧，带状态异常自动重试 -------
static HAL_StatusTypeDef hsc_read_with_retry(uint8_t idx,
                                             uint16_t *p_counts,
                                             uint16_t *t_counts,
                                             hsc_status_t *status_out)
{
    HAL_StatusTypeDef st = HAL_OK;
    uint8_t rx[HSC_FRAME_LEN];

    for (int attempt = 0; attempt < HSC_MAX_RETRIES; ++attempt) {
        st = hsc_xfer_locked(bus, idx, rx);
        if (st != HAL_OK) return st;

        uint16_t p = 0, t = 0; hsc_status_t s = HSC_STATUS_NORMAL;
        hsc_parse_frame(rx, &p, &t, &s);

        // 仅 NORMAL 认为有效；其它状态（COMMAND/STALE/DIAG）自动重读
        if (s == HSC_STATUS_NORMAL) {
            if (p_counts)   *p_counts   = p;
            if (t_counts)   *t_counts   = t;
            if (status_out) *status_out = s;
            return HAL_OK;
        }

        if (status_out) *status_out = s; // 把最后一次状态带出去
        osDelay(HSC_RETRY_DELAY_MS);
    }
    // 多次尝试仍非 NORMAL，则返回 HAL_OK 但 status!=NORMAL，或返回 HAL_ERROR？
    // 这里选择：返回 HAL_OK，调用者通过 status_out 判断数据有效性；如需改为 HAL_ERROR 可自行更改。
    return HAL_OK;
}

// ------- 公共 API -------

HAL_StatusTypeDef HSC_Init()
{
    memset(bus, 0, sizeof(*bus));
    bus->hspi = hscSPI;
    for (int i = 0; i < HSC_COUNT; ++i) {
        bus->cs_port[i] = cs_ports[i];
        bus->cs_pin[i]  = cs_pins[i];
        cs_high(bus->cs_port[i], bus->cs_pin[i]); // 空闲拉高
    }
    osMutexAttr_t attr = { .name = "hsc_bus_lock" };
    bus->bus_lock = osMutexNew(&attr);

    return (bus->bus_lock ? HAL_OK : HAL_ERROR);
}

// 兼容版：仅压力
HAL_StatusTypeDef HSC_ReadOne(uint8_t idx,
                              uint16_t *pressure_counts,
                              hsc_status_t *status_out)
{
    return HSC_ReadOneEx(idx, pressure_counts, NULL, status_out);
}

HAL_StatusTypeDef HSC_ReadAll(uint16_t pressure_counts[HSC_COUNT],
                              hsc_status_t status_arr[HSC_COUNT])
{
    return HSC_ReadAllEx(pressure_counts, NULL, status_arr);
}

// 扩展：压力+温度
HAL_StatusTypeDef HSC_ReadOneEx(uint8_t idx,
                                uint16_t *pressure_counts,
                                uint16_t *temp_counts,
                                hsc_status_t *status_out)
{
    if (!bus || !bus->hspi || idx >= HSC_COUNT) return HAL_ERROR;
    return hsc_read_with_retry(idx, pressure_counts, temp_counts, status_out);
}

HAL_StatusTypeDef HSC_ReadAllEx(uint16_t pressure_counts[HSC_COUNT],
                                uint16_t temp_counts[HSC_COUNT],
                                hsc_status_t status_arr[HSC_COUNT])
{
    if (!bus || !bus->hspi) return HAL_ERROR;

    HAL_StatusTypeDef st = HAL_OK;
    for (int i = 0; i < HSC_COUNT; ++i) {
        hsc_status_t s = HSC_STATUS_NORMAL;
        uint16_t p = 0, t = 0;
        st = hsc_read_with_retry(i, &p, &t, &s);
        if (st != HAL_OK) return st;
        if (pressure_counts) pressure_counts[i] = p;
        if (temp_counts)     temp_counts[i]     = t;
        if (status_arr)      status_arr[i]      = s;
    }
    return HAL_OK;
}

// ------- 简单数组工具（小窗口排序/平均） -------

static void isort_u16(uint16_t *a, uint8_t n)
{
    for (uint8_t i = 1; i < n; ++i) {
        uint16_t key = a[i];
        int8_t j = i - 1;
        while (j >= 0 && a[j] > key) { a[j+1] = a[j]; --j; }
        a[j+1] = key;
    }
}

static uint16_t mean_u16(const uint16_t *a, uint8_t n)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < n; ++i) sum += a[i];
    return (uint16_t)(sum / n);
}

static uint16_t median_u16(uint16_t *a, uint8_t n) // 就地排序
{
    isort_u16(a, n);
    if (n & 1) return a[n/2];
    // 偶数样本取中间两值平均
    return (uint16_t)(((uint32_t)a[n/2 - 1] + (uint32_t)a[n/2]) / 2);
}

// ------- 滤波读取 -------

HAL_StatusTypeDef HSC_ReadOneFiltered(uint8_t idx,
                                      uint8_t samples, uint32_t interval_ms,
                                      hsc_filter_mode_t mode,
                                      uint16_t *pressure_counts,
                                      uint16_t *temp_counts,
                                      hsc_status_t *final_status)
{
    if (!bus || !bus->hspi || idx >= HSC_COUNT || samples == 0) return HAL_ERROR;

    uint16_t pb[16], tb[16]; // 小窗口缓存；如需更大窗口，可调 16
    if (samples > 16) samples = 16;

    uint8_t ok = 0;
    hsc_status_t last_s = HSC_STATUS_NORMAL;

    for (uint8_t i = 0; i < samples; ++i) {
        uint16_t p = 0, t = 0; hsc_status_t s = HSC_STATUS_NORMAL;
        HAL_StatusTypeDef st = hsc_read_with_retry(idx, &p, &t, &s);
        last_s = s;
        if (st == HAL_OK && s == HSC_STATUS_NORMAL) {
            pb[ok] = p; tb[ok] = t; ok++;
        }
        if (interval_ms) osDelay(interval_ms);
    }

    if (final_status) *final_status = last_s;
    if (ok == 0) return HAL_OK; // 无有效样本，保持输出不变（或可返回 HAL_ERROR）

    if (mode == HSC_FILTER_MEAN) {
        if (pressure_counts) *pressure_counts = mean_u16(pb, ok);
        if (temp_counts)     *temp_counts     = mean_u16(tb, ok);
    } else if (mode == HSC_FILTER_MEDIAN) {
        if (pressure_counts) *pressure_counts = median_u16(pb, ok);
        if (temp_counts)     *temp_counts     = median_u16(tb, ok);
    } else { // NONE：取最后一次有效
        if (pressure_counts) *pressure_counts = pb[ok-1];
        if (temp_counts)     *temp_counts     = tb[ok-1];
    }
    return HAL_OK;
}

HAL_StatusTypeDef HSC_ReadAllFiltered(uint8_t samples, uint32_t interval_ms,
                                      hsc_filter_mode_t mode,
                                      uint16_t pressure_counts[HSC_COUNT],
                                      uint16_t temp_counts[HSC_COUNT],
                                      hsc_status_t status_arr[HSC_COUNT])
{
    if (!bus || !bus->hspi || samples == 0) return HAL_ERROR;

    // 为简化总线占用：逐通道做完整的采样窗口
    for (uint8_t i = 0; i < HSC_COUNT; ++i) {
        hsc_status_t s = HSC_STATUS_NORMAL;
        uint16_t p = 0, t = 0;
        HAL_StatusTypeDef st = HSC_ReadOneFiltered(i, samples, interval_ms, mode, &p, &t, &s);
        if (st != HAL_OK) return st;
        if (pressure_counts) pressure_counts[i] = p;
        if (temp_counts)     temp_counts[i]     = t;
        if (status_arr)      status_arr[i]      = s;
    }
    return HAL_OK;
}
