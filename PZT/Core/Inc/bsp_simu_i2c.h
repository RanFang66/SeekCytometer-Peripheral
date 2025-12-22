#ifndef __BSP_SIMU_I2C_H
#define __BSP_SIMU_I2C_H
#include "main.h"

void SI2C_Init(void);
void SI2C_Start(void);
void SI2C_Stop(void);
uint8_t SI2C_Send_Byte(uint8_t txd);
uint8_t SI2C_Read_Byte(uint8_t ack);
uint8_t SI2C_readData(uint8_t dev_addr, uint8_t *buffer, uint8_t length);
uint8_t SI2C_writeData(uint8_t dev_addr, uint8_t *buffer, uint8_t length);
uint8_t SI2C_writeReg(uint8_t dev_addr,uint8_t reg_addr,uint8_t data);
uint8_t SI2C_readReg(uint8_t dev_addr,uint8_t reg_addr);
uint8_t SI2C_writeRegs(uint8_t dev_addr,uint8_t reg_addr,uint8_t *buffer,uint8_t length);
uint8_t SI2C_readRegs(uint8_t dev_addr,uint8_t reg_addr,uint8_t *buffer,uint8_t length);
#endif

