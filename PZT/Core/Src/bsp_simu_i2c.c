#include "bsp_simu_i2c.h"



#define SCL_L HAL_GPIO_WritePin(DA_SCL_GPIO_Port, DA_SCL_Pin, GPIO_PIN_RESET)
#define SCL_H HAL_GPIO_WritePin(DA_SCL_GPIO_Port, DA_SCL_Pin, GPIO_PIN_SET)
#define SDA_L HAL_GPIO_WritePin(DA_SDA_GPIO_Port, DA_SDA_Pin, GPIO_PIN_RESET)
#define SDA_H HAL_GPIO_WritePin(DA_SDA_GPIO_Port, DA_SDA_Pin, GPIO_PIN_SET)
#define SCL_IN HAL_GPIO_ReadPin(DA_SCL_GPIO_Port, DA_SCL_Pin)
#define SDA_IN HAL_GPIO_ReadPin(DA_SDA_GPIO_Port, DA_SDA_Pin)
static void SI2C_delaynus()
{
	uint32_t i = 32;
	while(i--);
}
void SI2C_Init()
{
//	GPIO_InitTypeDef GPIO_InitStructure;
//	
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD, ENABLE);
//	
//  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
//  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
//  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;   
//  GPIO_Init(GPIOC, &GPIO_InitStructure);
//	
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;   
//  GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	SCL_H;
	SDA_H;
}
void SI2C_Start()
{
	SCL_H;
	SDA_H;
	SI2C_delaynus();
	SDA_L;
	SI2C_delaynus();
	SCL_L;
}	  
void SI2C_Stop()
{
	SCL_L;
	SDA_L;
 	SI2C_delaynus();
	SCL_H;
	SI2C_delaynus();	
	SDA_H;
	SI2C_delaynus();						   	
}

static void SI2C_Ack()
{
	SCL_L;
	SDA_L;
	SI2C_delaynus();
	SCL_H;
	SI2C_delaynus();
	SCL_L;
}	    
static void SI2C_NAck()
{
	SCL_L;
	SDA_H;
	SI2C_delaynus();
	SCL_H;
	SI2C_delaynus();
	SCL_L;
}					 				     	  
uint8_t SI2C_Send_Byte(uint8_t txd)
{                        
  uint8_t t;
	SCL_L;
  for(t = 0; t < 8; t++){              
    if(txd&0x80)
			SDA_H;
		else
			SDA_L;
    txd <<= 1; 	  
		SI2C_delaynus();
		SCL_H;
		SI2C_delaynus();
		SCL_L;
//		SI2C_delaynus();
  }
	
	SDA_H;
	SI2C_delaynus();	   
	SCL_H;
	SI2C_delaynus();
	if(SDA_IN){
		SI2C_Stop();
		return 1;
	}
	SI2C_delaynus();
	SCL_L;
	return 0;
} 	    
uint8_t SI2C_Read_Byte(uint8_t ack)
{
	uint8_t i,receive=0;
	SDA_H;
  for(i=0;i<8;i++ )
	{
		SI2C_delaynus();
		SCL_H;
		SI2C_delaynus();
    receive<<=1;
    if(SDA_IN)
			receive++;
		SCL_L;
  }
  if(ack)
		SI2C_Ack();
  else
    SI2C_NAck();  
  return receive;
}

uint8_t SI2C_writeData(uint8_t dev_addr, uint8_t *buffer, uint8_t length)
{
	uint8_t i;
	SI2C_Start();
	if(SI2C_Send_Byte(dev_addr<<1)) 
		return 1;
	for(i=0;i<length;i++)
	{
		if(SI2C_Send_Byte(buffer[i])) 
			return 2;
	}
	SI2C_Stop(); 
	return 0;
}


uint8_t SI2C_writeRegs(uint8_t dev_addr,uint8_t reg_addr,uint8_t *buffer,uint8_t length)
{
	uint8_t i;
	SI2C_Start();
	if(SI2C_Send_Byte(dev_addr<<1)) 
		return 1;
	if(SI2C_Send_Byte(reg_addr)) 
		return 2;
	for(i=0;i<length;i++)
	{
		if(SI2C_Send_Byte(buffer[i])) 
			return 3;
	}
	SI2C_Stop(); 
	return 0;
}

uint8_t SI2C_readData(uint8_t dev_addr, uint8_t *buffer, uint8_t length)
{
	uint8_t   i;
	SI2C_Start();
	if(SI2C_Send_Byte(0x01 | dev_addr<<1)) 
		return 1;
	SI2C_delaynus();
	SCL_H;
	i=10;
	while(i--){
		if(SCL_IN)			break;
		SI2C_delaynus();
	}
	if(!SCL_IN)
		return 2;
	for(i = 0;i < length;i++)
	{
		buffer[i]=SI2C_Read_Byte((i==(length-1))?0:1);
	}
	SI2C_Stop();
	return 0;
}


uint8_t SI2C_readRegs(uint8_t dev_addr,uint8_t reg_addr,uint8_t *buffer,uint8_t length)
{
	uint8_t   i;
	SI2C_Start();
	if(SI2C_Send_Byte(dev_addr<<1)) 
		return 1;
	if(SI2C_Send_Byte(reg_addr)) 
		return 2;
	SI2C_delaynus();
	SI2C_Start();
	if(SI2C_Send_Byte(0x01|dev_addr<<1)) 
		return 3;
	SI2C_delaynus();
	SCL_H;
	i=10;
	while(i--){
		if(SCL_IN)			break;
		SI2C_delaynus();
	}
	if(!SCL_IN)
		return 4;
	for(i = 0;i < length;i++)
	{
		buffer[i]=SI2C_Read_Byte((i==(length-1))?0:1);
	}
	SI2C_Stop();
	return 0;
}

uint8_t SI2C_writeReg(uint8_t dev_addr,uint8_t reg_addr,uint8_t data)
{
	return SI2C_writeRegs(dev_addr, reg_addr, &data, 1);
}

uint8_t SI2C_readReg(uint8_t dev_addr,uint8_t reg_addr)
{
	uint8_t data;
	if(SI2C_readRegs(dev_addr, reg_addr, &data, 1))
		return 0;
	return data;
}
