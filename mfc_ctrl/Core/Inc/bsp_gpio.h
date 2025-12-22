/*
 * bsp_gpio.h
 *
 *  Created on: 2025年11月27日
 *      Author: ranfa
 */

#ifndef INC_BSP_GPIO_H_
#define INC_BSP_GPIO_H_

#include "gpio.h"


#define SENSOR_SPI_SS1_GPIO		(GPIOC)
#define SENSOR_SPI_SS1_PIN		(GPIO_PIN_8)

#define SENSOR_SPI_SS2_GPIO		(GPIOC)
#define SENSOR_SPI_SS2_PIN		(GPIO_PIN_9)

#define SENSOR_SPI_SS3_GPIO		(GPIOA)
#define SENSOR_SPI_SS3_PIN		(GPIO_PIN_8)

#define SENSOR_SPI_SS4_GPIO		(GPIOB)
#define SENSOR_SPI_SS4_PIN		(GPIO_PIN_3)

#define SENSOR_SPI_SS5_GPIO		(GPIOB)
#define SENSOR_SPI_SS5_PIN		(GPIO_PIN_4)

#define SENSOR_SPI_SS6_GPIO		(GPIOA)
#define SENSOR_SPI_SS6_PIN		(GPIO_PIN_1)

#define DAC_SPI_SYNC_GPIO		(GPIOB)
#define DAC_SPI_SYNC_PIN		(GPIO_PIN_12)


#define SOLE_VALVE_1_GPIO		(GPIOB)
#define SOLE_VALVE_1_PIN		(GPIO_PIN_5)

#define SOLE_VALVE_2_GPIO		(GPIOB)
#define SOLE_VALVE_2_PIN		(GPIO_PIN_6)

#define SOLE_VALVE_3_GPIO		(GPIOB)
#define SOLE_VALVE_3_PIN		(GPIO_PIN_7)

#define SOLE_VALVE_4_GPIO		(GPIOB)
#define SOLE_VALVE_4_PIN		(GPIO_PIN_8)

#define SOLE_VALVE_5_GPIO		(GPIOB)
#define SOLE_VALVE_5_PIN		(GPIO_PIN_9)



#endif /* INC_BSP_GPIO_H_ */
