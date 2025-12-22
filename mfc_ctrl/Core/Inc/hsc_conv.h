/*
 * hsc_conv.h
 *
 *  Created on: Oct 24, 2025
 *      Author: ranfa
 */

#ifndef INC_HSC_CONV_H_
#define INC_HSC_CONV_H_

#include <stdint.h>


// 归一化到 0..1（小于0/大于1会钳位）
float HSC_CountsToUnit_TF_A(uint16_t counts);                 // 压力：TF=A 映射到 0..1

// Transfer Function = A（常见出厂设定，Pmin→10%，Pmax→90%）的换算工具：
// counts: 14-bit 原始计数（0..16383）；返回单位同你传入的 Pmin/Pmax（kPa、psi、mbar 均可）。
float HSC_CountsToPressure_mbar(uint16_t counts);


float HSC_CountsToPressure_psi(uint16_t counts);


float HSC_SourceCountsToPressure_mbar(uint16_t counts);

// 温度：11-bit 计数 0..2047 映射到实际温度（线性）。具体 Tmin/Tmax 见器件的温度定义。
// 若无明确范围，可先按数据手册/应用笔记给出的温度满量程设置。
float HSC_TempCountsToCelsius(uint16_t t_counts);

#endif /* INC_HSC_CONV_H_ */
