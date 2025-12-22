/*
 * ntc_sensor.h
 *
 *  Created on: 2025年12月3日
 *      Author: ranfa
 */

#ifndef INC_NTC_SENSOR_H_
#define INC_NTC_SENSOR_H_

#include <stdint.h>


float NTC_ConvertToTemp(uint16_t adc_value);

#endif /* INC_NTC_SENSOR_H_ */
