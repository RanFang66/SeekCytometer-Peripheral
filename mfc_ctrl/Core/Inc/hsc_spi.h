/*
 * hsc_spi.h
 *
 *  Created on: Oct 24, 2025
 *      Author: ranfa
 */

#ifndef INC_HSC_SPI_H_
#define INC_HSC_SPI_H_

#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 设备数量与帧定义
#define HSC_COUNT        6
#define HSC_FRAME_LEN    4

// 重试与时序参数（可按需调整）
#ifndef HSC_MAX_RETRIES
#define HSC_MAX_RETRIES  3     // 状态异常（非 NORMAL）时的最大自动重读次数
#endif

#ifndef HSC_RETRY_DELAY_MS
#define HSC_RETRY_DELAY_MS  1  // 每次重读之间的间隔（ms）
#endif

// 过滤模式
typedef enum {
    HSC_FILTER_NONE   = 0,
    HSC_FILTER_MEAN   = 1,
    HSC_FILTER_MEDIAN = 2
} hsc_filter_mode_t;

// 状态码（HSC 常见定义：首字节高 2 位）
typedef enum {
    HSC_STATUS_NORMAL  = 0, // 00
    HSC_STATUS_COMMAND = 1, // 01
    HSC_STATUS_STALE   = 2, // 10
    HSC_STATUS_DIAG    = 3  // 11
} hsc_status_t;

// SPI 总线实例（共用 SPI3，不同 CS）
typedef struct {
    SPI_HandleTypeDef *hspi;                  // &hspi3
    GPIO_TypeDef *cs_port[HSC_COUNT];         // PC, PC, PA, PB, PB, PA
    uint16_t     cs_pin [HSC_COUNT];          // 8, 9, 8, 3, 4, 1
    osMutexId_t  bus_lock;                    // SPI 互斥
} hsc_bus_t;

// 初始化
HAL_StatusTypeDef HSC_Init();

// ======== 基础读取（保持兼容） ========
// 仅压力（14-bit）+ 状态（可为 NULL），不返回温度
HAL_StatusTypeDef HSC_ReadOne(uint8_t idx,
                              uint16_t *pressure_counts,
                              hsc_status_t *status_out);

HAL_StatusTypeDef HSC_ReadAll(uint16_t pressure_counts[HSC_COUNT],
                              hsc_status_t status_arr[HSC_COUNT]);

// ======== 扩展读取：含温度（11-bit） ========
HAL_StatusTypeDef HSC_ReadOneEx(uint8_t idx,
                                uint16_t *pressure_counts,
                                uint16_t *temp_counts,
                                hsc_status_t *status_out);

HAL_StatusTypeDef HSC_ReadAllEx(uint16_t pressure_counts[HSC_COUNT],
                                uint16_t temp_counts[HSC_COUNT],
                                hsc_status_t status_arr[HSC_COUNT]);

// ======== 滤波读取（均值/中值；一次采样 N 次） ========
HAL_StatusTypeDef HSC_ReadOneFiltered(uint8_t idx,
                                      uint8_t samples, uint32_t interval_ms,
                                      hsc_filter_mode_t mode,
                                      uint16_t *pressure_counts,
                                      uint16_t *temp_counts,
                                      hsc_status_t *final_status);

HAL_StatusTypeDef HSC_ReadAllFiltered(uint8_t samples, uint32_t interval_ms,
                                      hsc_filter_mode_t mode,
                                      uint16_t pressure_counts[HSC_COUNT],
                                      uint16_t temp_counts[HSC_COUNT],
                                      hsc_status_t status_arr[HSC_COUNT]);

#ifdef __cplusplus
}
#endif


#endif /* INC_HSC_SPI_H_ */
