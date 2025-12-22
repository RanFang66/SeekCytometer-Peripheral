/*
 * modbus_gateway.c
 *
 *  Created on: 2025年11月26日
 *      Author: ranfa
 */


#include "modbus_gateway.h"

uint16_t MB_CalculateCRC16(const uint8_t *buffer, uint16_t length)
{
	uint16_t crc = 0xFFFF;
	for (uint16_t i = 0; i < length; i++) {
		crc ^= buffer[i];
		for (int j = 0; j < 8; j++) {
			if (crc & 1) crc = (crc >> 1) ^ 0xA001;
			else crc >>= 1;
		}
	}
	return crc;
}
