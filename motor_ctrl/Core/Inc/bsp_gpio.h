/*
 * bsp_gpio.h
 *
 *  Created on: 2025年11月27日
 *      Author: ranfa
 */

#ifndef INC_BSP_GPIO_H_
#define INC_BSP_GPIO_H_

#include "gpio.h"


#define CHURN_MOTOR_DIR_GPIO 		(GPIOE)
#define CHURN_MOTOR_DIR_PIN			(GPIO_PIN_4)

#define CHURN_MOTOR_EN_GPIO			(GPIOE)
#define CHURN_MOTOR_EN_PIN			(GPIO_PIN_6)

/* DEPRECATED: the cover mechanism was removed by the hardware revision and the
 * module is compiled out (see ENABLE_COVER_CTRL in cover_control.h), so nothing
 * drives these pins - they stay at the reset level set by MX_GPIO_Init. */
#define COVER_MOTOR_EN_GPIO			(GPIOC)
#define COVER_MOTOR_EN_PIN			(GPIO_PIN_9)

#define COVER_MOTOR_DIR_GPIO		(GPIOA)
#define COVER_MOTOR_DIR_PIN			(GPIO_PIN_11)

#define COVER_MOTOR_FG_GPIO			(GPIOA)
#define COVER_MOTOR_FG_PIN			(GPIO_PIN_12)


// Seal motor H-bridge power enable (M1_Power)
#define SEAL_MOTOR_EN_GPIO			(GPIOC)
#define SEAL_MOTOR_EN_PIN			(GPIO_PIN_7)

/* Seal motor H-bridge inputs. IN1 is the PWM side (TIM1 CH3, see bsp_tim.h);
 * the GPIO entry below only exists so the pin level can be read back for
 * diagnostics. IN2 is a plain output, MX_GPIO_Init configures it as M6_DIR_Pin
 * (the pin the old seal driver used as its direction line). */
#define SEAL_MOTOR_IN1_GPIO			(GPIOA)
#define SEAL_MOTOR_IN1_PIN			(GPIO_PIN_10)

#define SEAL_MOTOR_IN2_GPIO			(GPIOA)
#define SEAL_MOTOR_IN2_PIN			(GPIO_PIN_8)

#define SEAL_MOTOR_FG_GPIO			(GPIOA)
#define SEAL_MOTOR_FG_PIN			(GPIO_PIN_9)

#define LASER_1_EN_GPIO				(GPIOC)
#define LASER_1_EN_PIN				(GPIO_PIN_5)

#define LASER_2_EN_GPIO				(GPIOA)
#define LASER_2_EN_PIN				(GPIO_PIN_6)

#define POWER_12V_EN_GPIO			(GPIOB)
#define POWER_12_EN_PIN				(GPIO_PIN_13)

#define POWER_15V_EN_GPIO			(GPIOB)
#define POWER_15V_EN_PIN			(GPIO_PIN_12)

#define MCP4728_I2C_SCL_GPIO		(GPIOE)
#define MCP4728_I2C_SCL_PIN			(GPIO_PIN_13)

#define MCP4728_I2C_SDA_GPIO		(GPIOE)
#define MCP4728_I2C_SDA_PIN			(GPIO_PIN_12)

#define MCP4728_I2C_LDAC_GPIO		(GPIOE)
#define MCP4728_I2C_LDAC_PIN		(GPIO_PIN_11)

#define COVER_CLOSED_GPIO			(GPIOD)
#define COVER_CLOSED_PIN			(GPIO_PIN_0)

#define COVER_ALMOST_CLOSED_GPIO	(GPIOC)
#define COVER_ALMOST_CLOSED_PIN		(GPIO_PIN_12)


#define COVER_ALMOST_OPENED_GPIO	(GPIOC)
#define COVER_ALMOST_OPENED_PIN		(GPIO_PIN_11)


#define COVER_OPENED_GPIO			(GPIOC)
#define COVER_OPENED_PIN			(GPIO_PIN_10)

#define SEAL_PUSHED_GPIO			(GPIOC)
#define SEAL_PUSHED_PIN				(GPIO_PIN_8)




#endif /* INC_BSP_GPIO_H_ */
