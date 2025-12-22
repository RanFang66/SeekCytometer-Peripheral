/*
 * ntc_sensor.c
 *
 *  Created on: 2025年12月3日
 *      Author: ranfa
 */


#include "ntc_sensor.h"
#include <math.h>




/* 预计算的ADC值数组，对应温度从-40°C到70°C，ADC值降序排列 */
const uint16_t adc_value_array[] = {
	    3978, 3970, 3962, 3953, 3944, 3935, 3924, 3914, 3902, 3890,			// -40 ~ -31
	    3878, 3865, 3851, 3836, 3821, 3805, 3789, 3771, 3753, 3734,			// -30 ~ -21
	    3714, 3694, 3673, 3650, 3627, 3603, 3578, 3553, 3526, 3499,			// -20 ~ -11
	    3470, 3441, 3411, 3379, 3347, 3314, 3281, 3246, 3210, 3174,			// -10 ~ -1
	    3137, 3099, 3060, 3021, 2981, 2940, 2898, 2857, 2814, 2771,			// 0 ~9
	    2727, 2683, 2639, 2594, 2549, 2504, 2458, 2412, 2367, 2321,			// 10 ~ 19
	    2275, 2229, 2184, 2138, 2093, 2048, 2003, 1958, 1914, 1870,			// 20 ~ 29
	    1826, 1783, 1741, 1699, 1657, 1617, 1576, 1536, 1497, 1459,			// 30 ~ 39
	    1421, 1384, 1348, 1312, 1277, 1242, 1209, 1176, 1144, 1112,			// 40 ~ 49
	    1082, 1052, 1022, 994, 966, 939, 912, 886, 861, 836,				// 50 ~ 59
	    813, 789, 767, 745, 723, 703, 682, 663, 644, 625,					// 60 ~ 69
	    607																	// 70
};

#define TEMP_START 	(-40) // 查找表起始温度
#define TEMP_END 	(70)    // 查找表结束温度
#define ARRAY_SIZE (sizeof(adc_value_array) / sizeof(adc_value_array[1]))

/**
 * @brief 使用二分查找法定位ADC值在数组中的索引区间
 * @param adc_value 要查找的ADC采样值
 * @return 返回的索引值，满足 adc_value_array[index] <= adc_value < adc_value_array[index-1]
 */
int find_adc_index(uint16_t adc_value) {
    int low = 0;
    int high = ARRAY_SIZE - 1;
    int mid = 0;

    // 边界检查：如果ADC值大于等于最大值，说明温度低于测量下限
    if (adc_value >= adc_value_array[low]) {
        return 0;
    }
    // 边界检查：如果ADC值小于等于最小值，说明温度高于测量上限
    if (adc_value <= adc_value_array[high]) {
        return high;
    }

    // 二分查找核心逻辑[6](@ref)[7](@ref)
    while (low <= high) {
        mid = low + (high - low) / 2; // 防止溢出的写法

        if (adc_value == adc_value_array[mid]) {
            // 找到精确匹配，但为了后续插值，返回mid+1更合适，因为需要两个点
            return (mid < ARRAY_SIZE - 1) ? mid + 1 : mid;
        }

        if (adc_value > adc_value_array[mid]) {
            // ADC值在左半部分（因为数组降序排列）[4](@ref)
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    /* 循环结束后，low指向第一个大于adc_value的元素的索引。
       由于数组降序排列，我们需要的是第一个小于等于adc_value的元素的索引，即low-1。
       但为了插值，我们返回low，使其满足 adc_value_array[low] <= adc_value < adc_value_array[low-1] */
    return low;
}

/**
 * @brief 线性插值计算
 * @param x0, x1 已知X坐标（这里是ADC值）
 * @param y0, y1 已知Y坐标（这里是温度值）
 * @param x 当前的X坐标（当前ADC采样值）
 * @return 插值计算得到的Y坐标（温度值）
 */
float linear_interpolate(uint16_t x0, uint16_t x1, float y0, float y1, uint16_t x) {
    // 防止除零错误
    if (x0 == x1) return y0;
    // 线性插值公式: y = y0 + ( (y1 - y0) * (x - x0) ) / (x1 - x0)
    return y0 + ((y1 - y0) * (x - x0)) / (x1 - x0);
}

/**
 * @brief 根据ADC采样值计算温度的主函数
 * @param adc_value 12位ADC采样值（0-4095）
 * @return 计算得到的温度值，单位摄氏度
 */
float NTC_ConvertToTemp(uint16_t adc_value) {
    int index;

    // 1. 边界情况处理
    if (adc_value >= adc_value_array[0]) {
        return (float)TEMP_START; // 温度过低，返回下限值
    }
    if (adc_value <= adc_value_array[ARRAY_SIZE - 1]) {
        return (float)TEMP_END; // 温度过高，返回上限值
    }

    // 2. 使用二分查找定位索引[1](@ref)[6](@ref)
    index = find_adc_index(adc_value);

    // 3. 处理索引边界，确保能进行插值（需要index和index-1两个点）
    if (index <= 0) index = 1;
    if (index >= ARRAY_SIZE - 1) index = ARRAY_SIZE - 1;

    // 4. 获取用于插值的两个数据点
    uint16_t adc_low = adc_value_array[index];      // 较小的ADC值（对应较高温度）
    uint16_t adc_high = adc_value_array[index - 1];  // 较大的ADC值（对应较低温度）

    float temp_high = (float)(TEMP_START + index);     // 较高的温度值
    float temp_low = (float)(TEMP_START + index - 1);   // 较低的温度值

    // 5. 关键：进行线性插值[3](@ref)
    // 注意：在ADC-温度曲线上，ADC值与温度是负相关，所以插值方向是反的
    float temperature = linear_interpolate(adc_low, adc_high, temp_high, temp_low, adc_value);

    return temperature;
}
