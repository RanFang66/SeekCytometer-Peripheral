/*
 * bsp_gpio.h
 *
 *  Created on: 2026年8月31日
 *      Author: ranfa
 *
 *  Role to pin mapping. Every pin is configured in led_ctrl.ioc; this header
 *  only renames them semantically, so that upper layers do not depend on the
 *  spelling of a CubeMX label and a regeneration touches one file.
 */

#ifndef INC_BSP_GPIO_H_
#define INC_BSP_GPIO_H_

#include "main.h"

/* ---- TLC5957 serial interface (bit-banged, see design doc section 6.1) ---- */
#define TLC_SCLK_PORT		TLC_SCLK_GPIO_Port		// PA5
#define TLC_SCLK_PIN		TLC_SCLK_Pin
#define TLC_LAT_PORT		TLC_LAT_GPIO_Port		// PA6
#define TLC_LAT_PIN			TLC_LAT_Pin
#define TLC_SIN_PORT		TLC_SIN_GPIO_Port		// PA7
#define TLC_SIN_PIN			TLC_SIN_Pin

/* ---- Door solenoid lock ---- */
#define LOCK_EN_PORT		lock_en_GPIO_Port		// PA4, pulled low by external R7 10K
#define LOCK_EN_PIN			lock_en_Pin
#define LOCK_FB_PORT		lock_feedback_GPIO_Port	// PA1, dry contact buffered through U2
#define LOCK_FB_PIN			lock_feedback_Pin

/* ---- Spare dry-contact input (connector P9) ---- */
#define AUX_IN_PORT			aux_in_GPIO_Port		// PA10
#define AUX_IN_PIN			aux_in_Pin

/* ---- Heartbeat test point ----
 * PA9 is unconnected on the schematic and is the only scope observation point
 * this board has. Once the firmware runs the Debug_Modbus configuration there
 * is no shell, so this pin is the only way to tell the main loop is alive.
 *
 * PA9 has been added to led_ctrl.ioc, but the checked-in generated code predates
 * that edit, so main.h does not define heartbeat_Pin yet. The fallback below and
 * the matching init in app.c disappear on their own the next time CubeMX
 * regenerates - nothing to clean up later. */
#ifndef heartbeat_Pin
#define heartbeat_Pin			GPIO_PIN_9
#define heartbeat_GPIO_Port		GPIOA
#define BSP_HEARTBEAT_NEEDS_INIT	1				/* see App_Init() */
#endif

#define HEARTBEAT_PORT		heartbeat_GPIO_Port
#define HEARTBEAT_PIN		heartbeat_Pin

#endif /* INC_BSP_GPIO_H_ */
