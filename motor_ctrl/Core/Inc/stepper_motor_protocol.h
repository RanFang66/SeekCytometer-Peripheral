/*
 * stepper_motor_protocol.h
 *
 *  Created on: 2025年12月4日
 *      Author: ranfa
 */

#ifndef INC_STEPPER_MOTOR_PROTOCOL_H_
#define INC_STEPPER_MOTOR_PROTOCOL_H_


//Stepper Motor Register Definition
#define REG_CTRL_WORD    	0x0380U /* 6040h */
#define REG_STATUS_WORD     0x0381U /* 6041h */
#define REG_VCC_STD			0x0063U
#define REG_VCC_MIN			0x0094U
#define REG_USER_PARA_SAVE 	0x0026U
#define REG_CTRL_PROTOCOL	0x00B1U		// cia402=0
#define REG_MOTION_MODE 	0x03C2U		// 0x01=PP, 0x02=VM, 0x03=PV
#define REG_POS_ENCODER 	0x03C6U 	// 位置反馈编码器单位
#define REG_POS_FB			0x03C8U		// 位置范围(用户单位)
#define REG_POS_SET 		0x03E7U 	// 位置指令(用户单位)
#define REG_POS_POLARITY	0x03F3U 	// 位置和速度极性
#define REG_VEL_SET 		0x03F8U 	// 稳定速度
#define REG_ACC_SET 		0x03FCU 	// 加速度
#define REG_DESC_SET 		0x03FEU 	// 减速度
#define REG_VEL_FB			0x03D0U		// 实际速度反馈值(用户单位)
#define REG_VEL_FB_RPM		0x03D5U		// 实际速度反馈至(rpm单位)

#define REG_BIAS_THRESH		0x03CAU		// 位置误差过大阈值(用户单位)
#define REG_BIAS_TIMEWIN	0x03CCU		// 位置误差过大时间阈值(ms)
#define REG_ARIVE_THRESH	0x03CDU		// 位置到达阈值(用户单位)
#define REG_ARIVE_TIMEWIN   0x03CFU		// 位置到达时间窗口(ms)

#define REG_DI1_FUNC 		0x00D5U // 设置DI1功能
#define REG_DI1_LEVEL 		0x00D6U // 设置DI1电平逻辑
#define REG_DI2_FUNC 		0x00D7U // 设置DI2功能
#define REG_DI2_LEVEL 		0x00D8U // 设置DI2电平逻辑
#define REG_HOME_TYPE 		0x0416U // 原点回归方式选择
#define REG_HOME_HIGH_SPEED 0x0417U // 寻找限位开关速度
#define REG_HOME_LOW_SPEED 	0x0419U // 寻找零点速度
#define REG_HOME_ACC 		0x041BU // 归零加速度
#define REG_DI_STAT 		0x01E2U // 输入DI监控
#define REG_HOME_TIME_LIMIT 0x012EU // 原点回归超时定义 默认10s Y轴需要
#define REG_ERROR_CODE0 	0x0002U // 故障代码第一个


/* ---------- 控制字（Control Word）位定义（按位宏） -----------
   这些定义基于常见 CiA402 对象字典（仅作示例，必须与手册核对）：

   控制字（16-bit）常见位（bit numbering 从 0 开始，LSB 为 bit0）：
   - BIT 0: switch on
   - BIT 1: enable voltage
   - BIT 2: quick stop
   - BIT 3: enable operation
   - BIT 7: fault reset
   - BIT 8: halt (?)
   请以你设备手册的控制字位域为准。下面宏使用的是常见位位置。
*/
#define CW_BIT_SWITCH_ON          (1U << 0)  /* bit0 */
#define CW_BIT_ENABLE_VOLTAGE     (1U << 1)  /* bit1 */
#define CW_BIT_QUICK_STOP         (1U << 2)  /* bit2 */
#define CW_BIT_ENABLE_OPERATION   (1U << 3)  /* bit3 */
#define CW_BIT_FAULT_RESET        (1U << 7)  /* bit7 */
#define CW_BIT_HALT               (1U << 8)  /* bit8 */

// PP MODE CW BITS
#define CW_BIT_NEW_SET_POINT		(1U << 4)	// 0x10
#define CW_BIT_IMMEDIATE			(1U << 5)	// 0x20
#define CW_BIT_ABS_REL				(1U << 6)	// 0x40
#define CW_BIT_CHANGE_SET_POINT		(1U << 9)	// 0x0200

// HM MODE CW BITS
#define CW_BIT_HOME_START			(1U << 4)



#define CW_VAL_0x06   		0x0006U
#define CW_VAL_0x07   		0x0007U
#define CW_VAL_0x0F   		0x000FU

#define CW_REL_IMMEDIATE	0x006FU
#define CW_REL_WAIT			0x004FU
#define CW_ABS_IMMEDIATE	0x002FU
#define CW_ABS_WAIT			0x000FU




/* 便捷宏：按位构造控制字（例） */
#define CW_BUILD(switch_on, enable_volt, quick_stop, enable_op, fault_reset, halt, new_set_ack) \
    ( ((switch_on) ? CW_BIT_SWITCH_ON : 0) \
    | ((enable_volt) ? CW_BIT_ENABLE_VOLTAGE : 0) \
    | ((quick_stop) ? CW_BIT_QUICK_STOP : 0) \
    | ((enable_op) ? CW_BIT_ENABLE_OPERATION : 0) \
    | ((fault_reset) ? CW_BIT_FAULT_RESET : 0) \
    | ((halt) ? CW_BIT_HALT : 0) \
    | ((new_set_ack) ? CW_BIT_NEW_SETPOINT_ACK : 0) )


/* ---------- 状态字（Status Word）位定义（示例） ----------
   常见 CiA402 状态位（示例）：
   - SW_BIT_READY_TO_SWITCH_ON  bit0 (0x0001)
   - SW_BIT_SWITCHED_ON         bit1 (0x0002)
   - SW_BIT_OPERATION_ENABLED   bit2 (0x0004)
   - SW_BIT_FAULT               bit3 (0x0008)
   - SW_BIT_VOLTAGE_ENABLED     bit4 (0x0010)
   - SW_BIT_QUICK_STOP          bit5 (0x0020)
*/
#define SW_BIT_READY_TO_SWITCH_ON   (1U << 0) 	/* 0x0001 */
#define SW_BIT_SWITCHED_ON          (1U << 1) 	/* 0x0002 */
#define SW_BIT_OPERATION_ENABLED    (1U << 2) 	/* 0x0004 */
#define SW_BIT_FAULT                (1U << 3) 	/* 0x0008 */
#define SW_BIT_VOLTAGE_ENABLED      (1U << 4) 	/* 0x0010 */
#define SW_BIT_QUICK_STOP_ACTIVE    (1U << 5) 	/* 0x0020 */
#define SW_BIT_SWITCH_ON_DISABLED	(1U << 6) 	/* 0X0040 */
#define SW_BIT_WARNING				(1U << 7) 	/* 0x0080 */
#define SW_BIT_REACHED				(1U << 10) 	/* 0x0400 */
#define SW_BIT_HOME					(1U << 12)	/* 0x1000 */

// Control Mode Definition for REG_CTRL_MODE(0x00B1U)
#define CTRL_MODE_CIA402 		0x00			// CiA402模式
#define CTRL_MODE_NIMOTION_POS 	0x01			// Nimotion 位置模式
#define CTRL_MODE_NIMOTION_V	0x02			// Nimotion 速度模式
#define CTRL_MODE_NIMOTION_T	0x03			// Nimotion 转矩模式
#define CTRL_MODE_NIMOTION_OL	0x04			// Nimotion 开环模式

// Motion Mode Definition for REG_MOTION_MODE (0x03C2U)
#define MOTION_MODE_PP			0x01		// 位置轮廓模式
#define MOTION_MODE_VM 			0x02		// 速度模式
#define MOTION_MODE_PV			0x03		// 轮廓速度模式
#define MOTION_MODE_PT			0x04		// 轮廓转矩模式
#define MOTION_MODE_HM			0x06		// 原点回归模式
#define MOTION_MODE_IP			0x07		// 插补模式
#define MOTION_MODE_CSP			0x08		// 循环同步位置模式
#define MOTION_MODE_CSV			0x09		// 循环同步速度模式
#define MOTION_MODE_CST			0x0A		// 循环同步转矩模式

#define DI_FUNC_UNUSED			0U
#define DI_FUNC_HOME			31U
#define DI_FUNC_POS_LIMIT		14U
#define DI_FUNC_NEGA_LIMIT		15U

#define DI_LOW_VALID			0
#define DI_HIGH_VALID			1
#define DI_RISE_EDGE_VALID		2
#define DI_FALLING_EDGE_VALID	3
#define DI_ALL_EDGE_VALID		4


#define HOME_MODE_NEGA_LIMIT	17
#define HOME_MODE_POS_LIMIT		18
#define HOME_MODE_ORIGIN_19		19
#define HOME_MODE_ORIGIN_21		21

#define POS_POLARITY_POS		0x0000
#define POS_POLARITY_INVERT		0x0080

#endif /* INC_STEPPER_MOTOR_PROTOCOL_H_ */
