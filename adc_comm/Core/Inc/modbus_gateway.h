/*
 * modbus_gateway.h
 *
 *  Created on: 2025年11月26日
 *      Author: ranfa
 */

#ifndef INC_MODBUS_GATEWAY_H_
#define INC_MODBUS_GATEWAY_H_

#include <stdint.h>
#include <stdlib.h>

#define MB_LOCAL_ADDR				(0x11)
#define MB_MOTOR_ADDR				(0x22)
#define MB_MFC_ADDR					(0x33)
#define MB_PZT_ADDR					(0x44)

typedef enum {
	TARGET_LOCAL = 0,
	TARGET_MOTOR,
	TARGET_MFC,
	TARGET_PZT,
	TARGET_INVALID,
} target_t;


#define MB_MAX_FRAME_SIZE     		64     	// Modbus RTU max frame size
#define MB_DEFAULT_TIMEOUT_MS 		300			// Transaction timeout


#define MB_LOCAL_REG_START  	  	40001		// Start address of local holding register
#define MB_REG_LOCAL_COUNT			100     	// Size of the local Register Array (0-99)
#define MB_REG_BLOCK_COUNT			100

#define MB_MOTOR_REG_START			(MB_LOCAL_REG_START + MB_REG_LOCAL_COUNT)
#define MB_MFC_REG_START			(MB_MOTOR_REG_START + MB_REG_BLOCK_COUNT)
#define MB_PZT_REG_START			(MB_MFC_REG_START + MB_REG_BLOCK_COUNT)



/* --- Modbus Function Codes --- */
#define MB_FC_READ_HOLDING_REGS		0x03
#define MB_FC_WRITE_SINGLE_REG		0x06
#define MB_FC_WRITE_MULTI_REGS		0x10


/* --- Modbus Exception Codes --- */
#define MB_EX_ILLEGAL_FUNCTION    	0x01
#define MB_EX_ILLEGAL_DATA_ADDR   	0x02
#define MB_EX_ILLEGAL_DATA_VALUE  	0x03
#define MB_EX_SLAVE_DEVICE_FAILURE 	0x04


typedef enum {
	MB_OK = 0,
	MB_ERROR_TIMEOUT,
	MB_ERROR_CRC,
	MB_ERROR_SLAVE_EXCEP,
	MB_ERROR_WRONG_RESPONSE,
	MB_ERROR_BUSY,
	MB_ERROR_INVALID_PARA,
	MB_ERROR_NOMEM,
	MB_ERROR_HAL,
} MB_Status_t;


/* Helper for common modbus use */
uint16_t MB_CalculateCRC16(const uint8_t *buffer, uint16_t length);


#endif /* INC_MODBUS_GATEWAY_H_ */
