 /*
  * FreeModbus Libary: LPC214X Port
  * Copyright (C) 2007 Tiago Prado Lone <tiago@maxwellbohr.com.br>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Lesser General Public
  * License as published by the Free Software Foundation; either
  * version 2.1 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Lesser General Public License for more details.
  *
  * You should have received a copy of the GNU Lesser General Public
  * License along with this library; if not, write to the Free Software
  * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  *
  * File: $Id: port.c,v 1.1 2007/04/24 23:15:18 wolti Exp $
  */

/* ----------------------- System includes --------------------------------*/


/* ----------------------- Modbus includes ----------------------------------*/

#include "mb.h"
#include "mbport.h"
#include "step_motor_driver.h"
#include "pzt_quantity_check.h"
#include "interface.h"

/* ----------------------- Variables ----------------------------------------*/
/******************************************************************************
                               定义Modbus相关参数
******************************************************************************/

#define REG_INPUT_START                          (USHORT)0x0000  	//起始寄存器
#define REG_INPUT_NREGS                          (USHORT)11  		//寄存器个数
#define REG_HOLDING_START                        (USHORT)40301  	//保持寄存器
#define REG_HOLDING_NREGS                        (USHORT)26  		//保持寄存器个数ַ

//
// 电机控制状态寄存器 L/H
// 电机当前位置寄存器 1/2
// PZT零点 1/2
// PZT零点容差 1/2
// PZT电压变化时间常数 1/2

static const USHORT usRegInputStart = REG_INPUT_START;
static USHORT* const usRegInputBuf[REG_INPUT_NREGS] = {
		&system_state.data,
		(USHORT*)&step_motor1.pos + 1,
		(USHORT*)&step_motor1.pos,
		(USHORT*)&step_motor2.pos + 1,
		(USHORT*)&step_motor2.pos,
		&pztq_check1.adc_baseline,
		&pztq_check1.adc_threshold,
		&pztq_check1.rc,
		&pztq_check2.adc_baseline,
		&pztq_check2.adc_threshold,
		&pztq_check2.rc
};

// 控制命令字 L/H
// 电机细分配置寄存器1/2		运行时设定脉冲细分
// 电机脉冲周期寄存器1/2		运行时设定脉冲速度
// 电机绝对位置寄存器 1/2
// 电机相对步数寄存器 1/2
// 压紧时运行步数寄存器1/2
// 压紧触发临界值寄存器 1/2
static const USHORT usRegHoldingStart = REG_HOLDING_START;
static USHORT* const usRegHoldingBuf[REG_HOLDING_NREGS] = {
		&system_ctrl.data,
		&m1_regs.msteps_set,
		&m2_regs.msteps_set,
		&m1_regs.period_set,
		&m2_regs.period_set,
		(USHORT*)&m1_regs.pos_set + 1,
		(USHORT*)&m1_regs.pos_set,
		(USHORT*)&m1_regs.step_set + 1,
		(USHORT*)&m1_regs.step_set,
		(USHORT*)&pztq_check1.trigger_dF,
		(USHORT*)&m2_regs.pos_set + 1,
		(USHORT*)&m2_regs.pos_set,
		(USHORT*)&m2_regs.step_set + 1,
		(USHORT*)&m2_regs.step_set,
		(USHORT*)&pztq_check2.trigger_dF,


		&system_state.data,
		(USHORT*)&step_motor1.pos + 1,
		(USHORT*)&step_motor1.pos,
		(USHORT*)&step_motor2.pos + 1,
		(USHORT*)&step_motor2.pos,
		&pztq_check1.adc_baseline,
		&pztq_check1.adc_threshold,
		&pztq_check1.rc,
		&pztq_check2.adc_baseline,
		&pztq_check2.adc_threshold,
		&pztq_check2.rc
};

/**
  *****************************************************************************
  * @Name   : 操作输入寄存器
  *
  * @Brief  : 对应功能码0x04 -> eMBFuncReadInputRegister
  *
  * @Input  : *pucRegBuffer：数据缓冲区，响应主机用
  *           usAddress:     寄存器地址
  *           usNRegs:       操作寄存器个数
  *
  * @Output : none
  *
  * @Return : Modbus状态信息
  *****************************************************************************
**/
eMBErrorCode eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
	eMBErrorCode eStatus = MB_ENOERR;
	int          iRegIndex = 0;
	
	//
	//判断地址合法性
	//
	if ((usAddress >= REG_INPUT_START) && (usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS))
	{
		iRegIndex = (int)(usAddress - usRegInputStart);

		while (usNRegs > 0)
		{
			*pucRegBuffer++ = (UCHAR)( (*(usRegInputBuf[iRegIndex])) >> 8);  //高8位字节
			*pucRegBuffer++ = (UCHAR)( (*(usRegInputBuf[iRegIndex])) & 0xFF); //低8位字节
			iRegIndex++;
			usNRegs--;
		}
	}
	else  //错误地址ַ
	{
		eStatus = MB_ENOREG;
	}
	
	return eStatus;
}

/**
  *****************************************************************************
  * @Name   : 操作保持寄存器
  *
  * @Brief  : 对应功能码0x06 -> eMBFuncWriteHoldingRegister
  *                    0x16 -> eMBFuncWriteMultipleHoldingRegister
  *                    0x03 -> eMBFuncReadHoldingRegister
  *                    0x23 -> eMBFuncReadWriteMultipleHoldingRegister
  *
  * @Input  : *pucRegBuffer：数据缓冲区，响应主机用
  *           usAddress:     寄存器地址
  *           usNRegs:       操作寄存器个数
  *           eMode:         功能码
  *
  * @Output : none
  *
  * @Return : Modbus状态信息
  *****************************************************************************
**/
eMBErrorCode eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode )
{
	eMBErrorCode eStatus = MB_ENOERR;
	int          iRegIndex = 0;
	//
	//判断地址是否合法
	//
	if((usAddress >= REG_HOLDING_START) && ((usAddress + usNRegs) <= (REG_HOLDING_START + REG_HOLDING_NREGS)))
	{
		iRegIndex = (int)(usAddress - usRegHoldingStart);
		
		
		//
		//根据功能码进行操作
		//
		switch(eMode)
		{
			case MB_REG_READ:  //读保持寄存器
					while(usNRegs > 0)
					{
						*pucRegBuffer++ = (uint8_t)(*(usRegHoldingBuf[iRegIndex]) >> 8);  //高8位字节
						*pucRegBuffer++ = (uint8_t)(*(usRegHoldingBuf[iRegIndex]) & 0xFF); //低8位字节
						iRegIndex++;
						usNRegs--;
					}                            
					break;
					
			case MB_REG_WRITE:  //写保持寄存器
					while(usNRegs > 0)
					{
						*usRegHoldingBuf[iRegIndex] = *pucRegBuffer++ << 8;  	//高8位字节
						*usRegHoldingBuf[iRegIndex] |= *pucRegBuffer++;  		//低8位字节
						iRegIndex++;
						usNRegs--;
					}
					break;
		}
	}
	else  //错误地址ַ
	{
		eStatus = MB_ENOREG;
	}

	return eStatus;
}

/**
  *****************************************************************************
  * @Name   : 操作线圈
  *
  * @Brief  : 对应功能码0x01 -> eMBFuncReadCoils
  *                    0x05 -> eMBFuncWriteCoil
  *                    0x15 -> 写多个线圈 eMBFuncWriteMultipleCoils
  *
  * @Input  : *pucRegBuffer：数据缓冲区，响应主机用
  *           usAddress:     寄存器地址
  *           usNRegs:       操作寄存器个数
  *           eMode:         功能码
  *
  * @Output : none
  *
  * @Return : Modbus状态信息
  *****************************************************************************
**/
eMBErrorCode eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils, eMBRegisterMode eMode )
{
	eMBErrorCode eStatus = MB_ENOERR;
	int          iRegIndex = 0;
	
	//
	//判断地址合法性
	//
	if ((usAddress >= REG_HOLDING_START) && ((usAddress + usNCoils) <= (REG_HOLDING_START + REG_HOLDING_NREGS)))
	{
		iRegIndex = (int)(usAddress - usRegHoldingStart);
		//
		//根据功能码操作
		//
		switch (eMode)
		{
			case MB_REG_READ:  //读取操作
				while (usNCoils > 0)
				{
					*pucRegBuffer++ = (uint8_t)(*usRegHoldingBuf[iRegIndex] >> 8);  //高8位字节
					*pucRegBuffer++ = (uint8_t)(*usRegHoldingBuf[iRegIndex] & 0xFF);  //低8位字节
					iRegIndex++;
					usNCoils--;
				}
				break;
				
			case MB_REG_WRITE:  //写操作
				while (usNCoils > 0)
				{
					*usRegHoldingBuf[iRegIndex] = *pucRegBuffer++ << 8;  //高8位字节
					*usRegHoldingBuf[iRegIndex] |= *pucRegBuffer++;  //低8位字节
					iRegIndex++;
					usNCoils--;
				}
				break;
		}
	}
	else  //错误地址
	{
		eStatus = MB_ENOREG;
	}
	
	return eStatus;
}

/**
  *****************************************************************************
  * @Name   : 操作离散寄存器
  *
  * @Brief  : 对应功能码0x02 -> eMBFuncReadDiscreteInputs
  *
  * @Input  : *pucRegBuffer：数据缓冲区，响应主机用
  *           usAddress:     寄存器地址
  *           usNRegs:       操作寄存器个数
  *
  * @Output : none
  *
  * @Return : Modbus状态信息
  *****************************************************************************
**/
eMBErrorCode eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
	pucRegBuffer = pucRegBuffer;
	
	return MB_ENOREG;
}

void	vMBPortTimersDelay( USHORT usTimeOutMS ){}
