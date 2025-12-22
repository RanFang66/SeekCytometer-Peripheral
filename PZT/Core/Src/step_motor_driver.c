#include "step_motor_driver.h"
#include "usart.h"
#include "tim.h"
#include "time.h"
#include "stdlib.h"

uint32_t tbl = 2;
uint32_t toff = 5;
uint32_t hstrt = 4;
uint32_t hend = 0;
/*
 * 电机的PWM时钟都是1MHz
 * 当前压紧的采样信号为约50us周期
 * 要加速的话 每一步调整一次速度 最好脉冲不超过10kHz
 * 每步调整速度 则速度变化不是时间均匀的 但好处是知道多少步范围有加速的必要
 * 要规定一个最大速度和 最大启动速度
 * 还要规定加速度和减速度
 * 还要算出或者是规定多远的距离才适合加速 搞定
 *
 * 位置初始化问题
 * 需要定义复位后0位置所在点0位置基于哪个限位开关某个边沿和方向
 * 需要定义实验开始前的准备位置
 * 需要定义压紧时移动的步长
 * */

StepMotor_t step_motor1 = {
	.uart = &huart3,
	.htim = &htim1,
	.Channel = TIM_CHANNEL_4,
	.channel_ccr = &TIM1->CCR4,
	.port_dir = M1_DIR_GPIO_Port,
//	.port_spread = M1_SPREAD_GPIO_Port,
	.port_sw = SW1_GPIO_Port,
	.port_enn = M1_ENN_GPIO_Port,
	.port_index = M1_INDEX_GPIO_Port,
	.port_diag = M1_DIAG_GPIO_Port,
	.port_lock = M1_LOCK_GPIO_Port,
	.port_l = L3_GPIO_Port,
	.port_0 = L2_GPIO_Port,
	.port_h = L1_GPIO_Port,
	.pin_dir = M1_DIR_Pin,
//	.pin_spread = M1_SPREAD_Pin,
	.pin_sw = SW1_Pin,
	.pin_enn = M1_ENN_Pin,
	.pin_index = M1_INDEX_Pin,
	.pin_diag = M1_DIAG_Pin,
	.pin_lock = M1_LOCK_Pin,
	.pin_l = L3_Pin,
	.pin_0 = L2_Pin,
	.pin_h = L1_Pin,
	.stop_condition = NULL,
	.pos = MLL_LOW
};

StepMotor_t step_motor2 = {
	.uart = &huart1,
	.htim = &htim3,
	.Channel = TIM_CHANNEL_1,
	.channel_ccr = &TIM3->CCR1,
	.port_dir = M2_DIR_GPIO_Port,
//	.port_spread = MOTOR2_SPREAD_GPIO_Port,
	.port_sw = SW2_GPIO_Port,
	.port_enn = M2_ENN_GPIO_Port,
	.port_index = M2_INDEX_GPIO_Port,
	.port_diag = M2_DIAG_GPIO_Port,
	.port_lock = M2_LOCK_GPIO_Port,
	.port_l = L6_GPIO_Port,
	.port_0 = L5_GPIO_Port,
	.port_h = L4_GPIO_Port,
	.pin_dir = M2_DIR_Pin,
//	.pin_spread = M2_SPREAD_Pin,
	.pin_sw = SW2_Pin,
	.pin_enn = M2_ENN_Pin,
	.pin_index = M2_INDEX_Pin,
	.pin_diag = M2_DIAG_Pin,
	.pin_lock = M2_LOCK_Pin,
	.pin_l = L6_Pin,
	.pin_0 = L5_Pin,
	.pin_h = L4_Pin,
	.stop_condition = NULL,
	.pos = MLL_LOW
};

static uint8_t swuart_calcCRC(uint8_t *datagram, uint8_t datagramLength)
{
	int i,j;
	uint8_t currentByte;
	uint8_t crc = 0;
	for (i = 0; i < datagramLength; i++) {
		currentByte = datagram[i];
		// Execute for all bytes of a message
		// Retrieve a byte to be sent from Array
		for (j = 0; j < 8; j++) {
			if ((crc >> 7) ^ (currentByte & 0x01)) {  // update CRC based result of XOR operation
				crc = (crc << 1) ^ 0x07;
			} else {
				crc = (crc << 1);
			}
			currentByte = currentByte >> 1;
		} // for CRC bit
	} // for message byte
	return crc;
}

static void swuart_write_reg(UART_HandleTypeDef *huart, uint8_t reg, uint32_t data)
{
	uint8_t cmd[8];
	cmd[0] = 0x55;
	cmd[1] = 0;
	cmd[2] = 0x80 | reg;
	cmd[3] = data >> 24;
	cmd[4] = data >> 16;
	cmd[5] = data >> 8;
	cmd[6] = data;
	cmd[7] = swuart_calcCRC(cmd , 7);
	HAL_UART_Transmit(huart, cmd, 8, 100);
}

static uint32_t swuart_read_reg(UART_HandleTypeDef *huart, uint8_t reg)
{
	uint32_t data = 0;
	uint8_t cmd[8];
	cmd[0] = 0x55;
	cmd[1] = 0;
	cmd[2] = reg;
	cmd[3] = swuart_calcCRC(cmd , 3);
	HAL_UART_Transmit(huart, cmd, 4, 10);
	if (huart->Instance->SR & UART_FLAG_RXNE) {
		(void)huart->Instance->DR;
	}
	HAL_UART_Receive(huart, cmd, 8, 100);
	if (cmd[7] == swuart_calcCRC(cmd , 7)) {
		data = (cmd[3] << 24) | (cmd[4] << 16) | (cmd[5] << 8) | (cmd[6]);
	}
	return data;
}

void step_motor_init(StepMotor_t *motor)
{
	uint32_t reg = swuart_read_reg(motor->uart, REG_GCONF);
	HAL_Delay(2);
	reg &= 0xFFE;
	reg |= 0x80 | 0x40;
	swuart_write_reg(motor->uart, REG_GCONF, reg);
//
	HAL_Delay(2);
	reg = (2 << 16) | (16 << 8) | 3;
	swuart_write_reg(motor->uart, REG_IHOLD_IRUN, reg);

	HAL_Delay(2);
	reg = swuart_read_reg(motor->uart, REG_PWMCONF);
	HAL_Delay(2);
	reg &= 0xFFF0FFFF;
	reg |= 0xd << 16;
	swuart_write_reg(motor->uart, REG_PWMCONF, reg);
	HAL_Delay(2);
	swuart_write_reg(motor->uart, REG_TPWMTHRS, 200);

//	swuart_write_reg(REG_CHOPCONF, 0x10045);

//	swuart_write_reg(motor->uart, REG_SGTHRS, 3);
	motor->step = 0;

	HAL_GPIO_WritePin(motor->port_enn, motor->pin_enn, GPIO_PIN_SET);
	HAL_GPIO_WritePin(motor->port_lock, motor->pin_lock, GPIO_PIN_SET);
	HAL_Delay(20);

	step_motor_set_dir(motor, MDIR_UP);
//	if (step_motor_get_limit_h(motor)) {
//		motor->pos = MLL_HIGH;
//	} else if (step_motor_get_limit_m(motor)) {
//		motor->pos = MLL_MID;
//	} else {
//		motor->pos = MLL_LOW;
//	}
	motor->pos_valid = 0;
	motor->L3bits = step_motor_get_limit_h(motor) << 2;
	motor->L3bits |= step_motor_get_limit_m(motor) << 1;
	motor->L3bits |= step_motor_get_limit_l(motor) << 0;

	HAL_GPIO_WritePin(motor->port_lock, motor->pin_lock, GPIO_PIN_RESET);

}


void step_motor_set_period_steps(StepMotor_t *motor, uint16_t period, uint32_t steps)
{
	// 假设只给出加速度，没有最终速度限制
	// 此时目标速度对应的脉冲周期为p  则 (pmaxup - p) / ramp_up + (pmaxdown - p) / ramp_down = steps
	// p = (pmaxup/ramp_up + pmaxdown/ramp_down - steps) / (1/ramp_up + 1/ramp_down)
	motor->step = steps >> motor->msteps;
	motor->period_set = period < MOTOR_MIN_PERIOD ? MOTOR_MIN_PERIOD : period;

	motor->run_pulse_period =  (motor->period_set > MOTOR_START_PERIOD) ? motor->period_set : MOTOR_START_PERIOD;
	if (motor->period_set > MOTOR_STOP_PERIOD) {
		motor->step_ramp_down = 0;
	} else if (steps >= MOTOR_RAMP_MAX_STEPS) {
		motor->step_ramp_down = (MOTOR_STOP_PERIOD - motor->period_set) / MOTOR_RAMP_DOWN;
	} else {
		float lim_p = (MOTOR_RAMP_MAX_STEPS - steps) / MOTOR_K;
		lim_p = (lim_p < motor->period_set) ? motor->period_set : lim_p;
		motor->step_ramp_down = (MOTOR_STOP_PERIOD - lim_p) / MOTOR_RAMP_DOWN;
	}

}

void step_motor_set_dir(StepMotor_t *motor, int8_t dir)
{
	motor->dir = dir;
	if (dir == MDIR_UP)
		HAL_GPIO_WritePin(motor->port_dir, motor->pin_dir, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(motor->port_dir, motor->pin_dir, GPIO_PIN_RESET);
}

//int8_t step_motor_get_dir(StepMotor_t *motor)
//{
//	return HAL_GPIO_ReadPin(motor->port_dir, motor->pin_dir) ? MDIR_UP : MDIR_DOWN;
//}

// 值越大 速度越快
void step_motor_set_mstep(StepMotor_t *motor, uint8_t mstep)
{
	uint32_t reg;
	if (mstep > 8)
		mstep = 8;
	if (mstep == motor->msteps)
		return;
	motor->msteps = mstep;
//	reg = swuart_read_reg(REG_CHOPCONF);
//	reg &= 0xF0FFFFFF;
//	reg = (mstep << 24) | (1 << 17) |
//			(tbl << 15) | (hend << 7) |
//			(hstrt << 4) | toff;
	reg = (mstep << 24) | 0x30045;
	swuart_write_reg(motor->uart, REG_CHOPCONF, reg);
}

void step_motor_start(StepMotor_t *motor)
{
	HAL_GPIO_WritePin(motor->port_enn, motor->pin_enn, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(motor->port_lock, motor->pin_lock, GPIO_PIN_SET);

	motor->htim->Instance->ARR = (uint32_t)motor->run_pulse_period;
	HAL_TIM_PWM_Start(motor->htim, motor->Channel);
	HAL_TIM_Base_Start_IT(motor->htim);

	LED_H;

}

void step_motor_stop(StepMotor_t *motor)
{
	motor->step = 0;
	HAL_TIM_PWM_Stop(motor->htim, motor->Channel);
	HAL_GPIO_WritePin(motor->port_lock, motor->pin_lock, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(motor->port_enn, motor->pin_enn, GPIO_PIN_SET);

	LED_L;
}

// 电机复位 查找零点
// 轮询状态 返回1 即可结束
// 当前以中间限位开关上边沿跳变点为原点

uint8_t step_motor_reset(StepMotor_t *motor)
{
	// 检查高点限位
	switch(motor->pos_valid) {
	case 0: // 初始化 启动
		if (bsp_time_more(motor->timestamp, 50)) {
			bsp_time_clear(motor->timestamp);
			if (step_motor_get_limit_h(motor)) {
				// 如果高位限位开关触发则向下寻找原点
				step_motor_set_dir(motor, MDIR_DOWN);
			} else {
				// 否则默认向上寻找原点
				step_motor_set_dir(motor, MDIR_UP);
			}
			step_motor_set_mstep(motor, 6);
			step_motor_set_period_steps(motor, 1000, 30000);
			step_motor_start(motor);
			motor->pos_valid = 1;
		}
		break;
	case 1:
		if (step_motor_get_limit_h(motor)) {
			// 如果高位限位开关触发则继续向下寻找原点
			step_motor_set_dir(motor, MDIR_DOWN);
			__disable_irq();
			motor->pos = 0;
			__enable_irq();
		} else if (motor->dir == MDIR_DOWN && step_motor_get_limit_l(motor)) {
			// 向下运动 最低限位触发 异常
			motor->pos_valid = 3;
		}
		// 如果复位过程移动位移太长 证明硬件异常
		if (abs(motor->pos) > 200 * 256 * 30) {
			motor->pos_valid = 3;
		}
		// 复位过程中 步数不是重点 无限补充
		if (motor->step < 10000) {
			__disable_irq();
			motor->step = 30000;
			__enable_irq();
		}


		break;
	case 2:
	case 3:
		step_motor_stop(motor);
		break;
	}
	return motor->pos_valid >= 2;
}

void step_motor_reset_init(StepMotor_t *motor)
{
	HAL_GPIO_WritePin(motor->port_lock, motor->pin_lock, GPIO_PIN_SET);
	motor->pos_valid = 0;
	motor->pos = 0;
	bsp_time_clear(motor->timestamp);
}




void step_motor_set_callback(StepMotor_t *motor, uint8_t (*callback)(void *), void* param)
{
	motor->stop_condition = callback;
	motor->param = param;
}

void step_motor_pwm_count(StepMotor_t *motor)
{
	// 刹车判断
	uint8_t stop = 0;
	uint8_t last_L3bits;
	if (motor->stop_condition)
		stop = motor->stop_condition(motor->param);
	if (motor->step) {
		motor->step--;
		if (motor->step == 0 || stop)
			step_motor_stop(motor);
	}
	// 加减速判断
	if (motor->step < motor->step_ramp_down) {
		motor->run_pulse_period += MOTOR_RAMP_DOWN;
		motor->htim->Instance->ARR = (uint32_t)motor->run_pulse_period;
		*motor->channel_ccr = (uint32_t)motor->run_pulse_period / 2;
	} else if (motor->run_pulse_period > motor->period_set) {
		motor->run_pulse_period -= MOTOR_RAMP_UP;
		motor->htim->Instance->ARR = (uint32_t)motor->run_pulse_period;
		*motor->channel_ccr = (uint32_t)motor->run_pulse_period / 2;
	}

	motor->pos += motor->dir * (1 << motor->msteps);
	last_L3bits = motor->L3bits & 2;
	motor->L3bits = (motor->port_0->IDR & motor->pin_0) ? 2 : 0;

	if (last_L3bits ^ motor->L3bits) { // 信号发生翻转
		if (motor->dir == MDIR_DOWN && motor->L3bits == 0) { // 下行 且下降沿
			motor->pos = 1;
			motor->pos_valid = 2;
//		} else if (motor->dir == MDIR_UP && motor->L3bits != 0) { // 上行且上升沿
//			motor->pos = 0;
//			motor->pos_valid = 2;
		}
	}

	motor->L3bits |= (motor->port_l->IDR & motor->pin_l) ? 1 : 0;
	motor->L3bits |= (motor->port_h->IDR & motor->pin_h) ? 4 : 0;
//	if (step_motor_get_limit_h(motor)) {
//		motor->pos = MLL_HIGH;
////		if (DIR_UP == step_motor_get_dir(motor))
////			step_motor_stop(motor);
//	} else if (step_motor_get_limit_l(motor)) {
//		motor->pos = MLL_LOW;
////		if (DIR_DOWN == step_motor_get_dir(motor))
////			step_motor_stop(motor);
//	} else if (step_motor_get_limit_m(motor)) {
//		motor->pos = MLL_MID;
//	} else {
//		if (DIR_UP == step_motor_get_dir(motor) && motor->pos > MLL_HIGH) {
//			motor->pos = (motor->pos + 1) / 2 * 2 - 1;
//		} else if (DIR_DOWN == step_motor_get_dir(motor) && motor->pos < MLL_LOW) {
//			motor->pos = motor->pos / 2 * 2 + 1;
//		}
//	}

}

uint8_t step_motor_get_limit_h(StepMotor_t *motor)
{
	return HAL_GPIO_ReadPin(motor->port_h, motor->pin_h) == GPIO_PIN_SET ? 1 : 0;
}

uint8_t step_motor_get_limit_m(StepMotor_t *motor)
{
	return HAL_GPIO_ReadPin(motor->port_0, motor->pin_0) == GPIO_PIN_SET ? 1 : 0;
}

uint8_t step_motor_get_limit_l(StepMotor_t *motor)
{
	return HAL_GPIO_ReadPin(motor->port_l, motor->pin_l) == GPIO_PIN_SET ? 1 : 0;
}

uint32_t step_motor_get_state(StepMotor_t *motor)
{
	return swuart_read_reg(motor->uart, 0x12);
}
