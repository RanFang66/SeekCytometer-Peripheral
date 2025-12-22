/*
 * modbus_defs.h
 *
 *  Created on: 2025年12月4日
 *      Author: ranfa
 */

#ifndef INC_MODBUS_DEFS_H_
#define INC_MODBUS_DEFS_H_

#define MB_MAX_FRAME_SIZE     		256     	// Modbus RTU max frame size
#define MB_DEFAULT_TIMEOUT_MS 		300			// Transaction timeout
#define MB_RX_BUFFER_SIZE			256

/* --- Modbus Function Codes --- */
#define MB_FC_READ_HOLDING_REGS		0x03
#define MB_FC_WRITE_SINGLE_REG		0x06
#define MB_FC_WRITE_MULTI_REGS		0x10



/* --- Modbus Exception Codes --- */
#define MB_EX_ILLEGAL_FUNCTION    	0x01
#define MB_EX_ILLEGAL_DATA_ADDR   	0x02
#define MB_EX_ILLEGAL_DATA_VALUE  	0x03
#define MB_EX_SLAVE_DEVICE_FAILURE 	0x04

#define MB_LOCAL_REG_START  	  	40101		// Start address of local holding register
#define MB_REG_LOCAL_COUNT			100     	// Size of the local Register Array (0-99)

#endif /* INC_MODBUS_DEFS_H_ */
