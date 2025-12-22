#ifndef __STEP_MOTOR_DRIVER_H
#define __STEP_MOTOR_DRIVER_H
#include "main.h"

#define MOTOR_DIR_H HAL_GPIO_WritePin(MOTOR_DIR_GPIO_Port, MOTOR_DIR_Pin, GPIO_PIN_SET)
#define MOTOR_DIR_L HAL_GPIO_WritePin(MOTOR_DIR_GPIO_Port, MOTOR_DIR_Pin, GPIO_PIN_RESET)

#define MOTOR_SPREAD_H HAL_GPIO_WritePin(MOTOR_SPREAD_GPIO_Port, MOTOR_SPREAD_Pin, GPIO_PIN_SET)
#define MOTOR_SPREAD_L HAL_GPIO_WritePin(MOTOR_SPREAD_GPIO_Port, MOTOR_SPREAD_Pin, GPIO_PIN_RESET)

#define MOTOR_ENN_H HAL_GPIO_WritePin(MOTOR_ENN_GPIO_Port, MOTOR_ENN_Pin, GPIO_PIN_SET)
#define MOTOR_ENN_L HAL_GPIO_WritePin(MOTOR_ENN_GPIO_Port, MOTOR_ENN_Pin, GPIO_PIN_RESET)

#define MOTOR_LOCK_H HAL_GPIO_WritePin(MOTOR_LOCK_GPIO_Port, MOTOR_LOCK_Pin, GPIO_PIN_SET)
#define MOTOR_LOCK_L HAL_GPIO_WritePin(MOTOR_LOCK_GPIO_Port, MOTOR_LOCK_Pin, GPIO_PIN_RESET)

#define MOTOR_INDEX_IN HAL_GPIO_ReadPin(MOTOR_INDEX_GPIO_Port, MOTOR_INDEX_Pin)
#define MOTOR_DIAG_IN HAL_GPIO_ReadPin(MOTOR_DIAG_GPIO_Port, MOTOR_DIAG_Pin)

#define REG_GCONF 0x00	//全局控制寄存器
#define REG_GSTAT 0x01  //全局状态寄存器
#define REG_SLAVECONF 0x03 // 修改串口收发间隔
#define REG_OTP_PROG 0x04  // 写存储参数
#define REG_OTP_READ 0x05 // 读存储参数
#define REG_IHOLD_IRUN	0x10// 抱轴电流和运行电流调节
#define REG_TPWMTHRS 0x13
#define REG_VACTUAL 0x22
#define REG_SGTHRS 0x40
#define REG_CHOPCONF 0x6C	// 配置细分等
#define REG_DRV_STATUS 0x6F	// 驱动状态寄存器
#define REG_PWMCONF 0x70 //

#define MDIR_UP (-1)
#define MDIR_DOWN (1)

typedef enum {
	MLL_HIGH = 0,
	MLL_HIGH_MID = 1,
	MLL_MID = 2,
	MLL_MID_LOW = 3,
	MLL_LOW = 4
} MOTOR_LIMIT_LOCATION;

typedef struct {
	UART_HandleTypeDef *uart;
	TIM_HandleTypeDef *htim;
	uint32_t Channel;
	__IO uint32_t* channel_ccr;
	uint8_t (*stop_condition)(void *);
	void *param;
	GPIO_TypeDef* port_dir;
//	GPIO_TypeDef* port_spread;
	GPIO_TypeDef* port_sw;
	GPIO_TypeDef* port_enn;
	GPIO_TypeDef* port_index;
	GPIO_TypeDef* port_diag;
	GPIO_TypeDef* port_lock;
	GPIO_TypeDef* port_l;
	GPIO_TypeDef* port_0;
	GPIO_TypeDef* port_h;
	uint16_t pin_dir;
//	uint16_t pin_spread;
	uint16_t pin_sw;
	uint16_t pin_enn;
	uint16_t pin_index;
	uint16_t pin_diag;
	uint16_t pin_lock;
	uint16_t pin_l;
	uint16_t pin_0;
	uint16_t pin_h;
	uint16_t msteps;
	uint32_t step;			// 目标步数剩余步数
	uint16_t period_set;		// 目标控制周期
	uint16_t step_ramp_down; // 刹车步数
	int8_t dir;

	uint8_t L3bits;
	uint8_t pos_valid;
	int32_t pos;
	uint32_t timestamp;
	float run_pulse_period; // 运行时控制周期
}StepMotor_t;

#define MOTOR_MIN_PERIOD 100
#define MOTOR_START_PERIOD 1000		// 启动速度对应的周期
#define MOTOR_STOP_PERIOD  1000		// 停止速度对应的周期
#define MOTOR_RAMP_UP	1.f		// 加速过程中每步减少的周期值
#define MOTOR_RAMP_DOWN 4.f		// 减速过程中每步增加的周期值
#define MOTOR_RAMP_MAX_STEPS (MOTOR_START_PERIOD / MOTOR_RAMP_UP + MOTOR_STOP_PERIOD / MOTOR_RAMP_DOWN)
#define MOTOR_K	(1.f / MOTOR_RAMP_UP + 1.f / MOTOR_RAMP_DOWN)		// 反正就是一个计算中的系数
typedef struct {
	uint16_t min_pwm_period;
	uint16_t min_pwm_start_period;
	float ramp_up;
	float ramp_down;
	int32_t pos_ready_1;
	int32_t pos_ready_2;
	uint16_t press_steps1;
	uint16_t press_steps2;
}MotorCtrlInfo_t;


extern StepMotor_t step_motor1, step_motor2;
void step_motor_init(StepMotor_t *motor);
void step_motor_reset_init(StepMotor_t *motor);
uint8_t step_motor_reset(StepMotor_t *motor);
void step_motor_set_dir(StepMotor_t *motor, int8_t dir);
int8_t step_motor_get_dir(StepMotor_t *motor);
void step_motor_set_mstep(StepMotor_t *motor, uint8_t mstep);
void step_motor_set_period_steps(StepMotor_t *motor, uint16_t period, uint32_t steps);
void step_motor_start(StepMotor_t *motor);
void step_motor_stop(StepMotor_t *motor);
void step_motor_pwm_count(StepMotor_t *motor);
void step_motor_set_callback(StepMotor_t *motor, uint8_t (*callback)(void*), void* param);
uint32_t step_motor_get_state(StepMotor_t *motor);

uint8_t step_motor_get_limit_h(StepMotor_t *motor);

uint8_t step_motor_get_limit_m(StepMotor_t *motor);

uint8_t step_motor_get_limit_l(StepMotor_t *motor);
#endif
