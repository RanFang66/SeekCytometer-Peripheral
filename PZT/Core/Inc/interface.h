#ifndef __INTERFACE_H
#define __INTERFACE_H
#include "main.h"
#include "usart.h"


#define USE_MODBUS
#ifdef USE_MODBUS
// 电机复位，绝对位置运行，相对位置运行 压紧 PZT零点更新 PZT参数校准 高低字节分别表示两个通道
typedef enum {
	MC_IDLE = 0,	// 空闲 / 停止
	MC_MOVE_ABS,	// 绝对位置运动
	MC_MOVE_REL,	// 相对位置运动
	MC_PRESS,		// 压紧
	MC_UPDATE_ORIGIN, // PZT 0点更新
	MC_UPDATE_RC,	// PZT 时间常数更新
	MC_MOTOR_RESET
} MODBUS_CMD;
typedef enum {
	MCS_INIT = 0,
	MCS_BUSY,
	MCS_DONE,
	MCS_ERROR
} MODBUS_CMD_STATE;
typedef union {
	uint16_t data;
	struct {
		uint16_t cmd1 : 6; // MODBUS_CMD
		uint16_t state1 : 2; // MODBUS_CMD_STATE
		uint16_t cmd2 : 6; // MODBUS_CMD
		uint16_t state2 : 2; // MODBUS_CMD_STATE
	} bits;
} SystemCtrlStateReg;

typedef struct {
	uint16_t msteps_set;
	uint16_t period_set;
	int32_t pos_set;
	int32_t step_set;
} MODBUS_ChannelRegs;
extern MODBUS_ChannelRegs m1_regs, m2_regs;

extern SystemCtrlStateReg system_ctrl, system_state;

void interface_run(void);
#else
void interface_start(void);

void interface_receive(uint16_t size);
void interface_send(void* buffer, uint16_t size);
void interface_run(void);
void press_stop(void);
#endif // USE_MODBUS
#endif
