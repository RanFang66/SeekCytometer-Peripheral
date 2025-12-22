#include "interface.h"
#include "string.h"
#include "time.h"
#include "pzt_current_check.h"
#include "pzt_quantity_check.h"
#include "dcdc_power_ctrl.h"
#include "step_motor_driver.h"
#include "tim.h"
#include "stdlib.h"
#ifdef USE_MODBUS
SystemCtrlStateReg system_ctrl;
SystemCtrlStateReg system_state;
MODBUS_ChannelRegs m1_regs = {
		.msteps_set = 2,
		.period_set = 1000
};

MODBUS_ChannelRegs m2_regs = {
		.msteps_set = 2,
		.period_set = 1000
};

static void motor_move_config(StepMotor_t *motor, int32_t steps, uint16_t period)
{
	if (steps > 0) {
		step_motor_set_dir(motor, MDIR_DOWN);
	} else if (steps < 0) {
		step_motor_set_dir(motor, MDIR_UP);
	} else
		return;

	step_motor_set_period_steps(motor, period, abs(steps));
	step_motor_start(motor);
}
int16_t x[10] = {836, 758, 676, 593, 513, 431, 348, 263, 180, 99};
int16_t y[10] = {100,  90,  80, 70,   60,  50,  40,  30,  20, 10};
static MODBUS_CMD_STATE run_cmd_idle(void)
{

	float k, b;
	linear_fit(x, y, 10, &k, &b);
	MODBUS_CMD_STATE ret = MCS_DONE;
	return ret;
}

static MODBUS_CMD_STATE run_cmd_move(MODBUS_CMD_STATE state, StepMotor_t *motor, int32_t steps, uint16_t period)
{
	MODBUS_CMD_STATE ret = state;
	switch(state) {
	case MCS_INIT:
		step_motor_set_callback(motor, NULL, NULL);
		motor_move_config(motor, steps, period);
		ret = MCS_BUSY;
		break;
	case MCS_BUSY:
		if (motor->step == 0) {
			ret = MCS_DONE;
		}
		break;
	case MCS_DONE:
	case MCS_ERROR:
		break;
	default:
		ret = MCS_ERROR;
		break;
	}
	return ret;
}

static  MODBUS_CMD_STATE run_cmd_press(MODBUS_CMD_STATE state, StepMotor_t *motor, PZTQ_CheckInfo * qcheck, int32_t steps, uint16_t period)
{
	MODBUS_CMD_STATE ret = state;
	switch(state) {
	case MCS_INIT:
		if (HAL_IS_BIT_SET(qcheck->adc_handle->Instance->CR2, ADC_CR2_ADON)) {
			if (bsp_time_more(qcheck->timestamp, 50)) {
				if (abs(qcheck->adc_value - qcheck->adc_baseline) < qcheck->adc_threshold) {
					pztq_check_reset(qcheck);
					step_motor_set_callback(motor, (uint8_t (*)(void *))pztq_check, (void *)qcheck);
					motor_move_config(motor, steps, period);
					ret = MCS_BUSY;
				}
			}
		} else {
			pztq_check_start(qcheck);
			bsp_time_clear(qcheck->timestamp);
		}

		break;
	case MCS_BUSY:
		if (motor->step == 0) {
			ret = qcheck->dF > qcheck->trigger_dF ? MCS_DONE : MCS_ERROR;
			pztq_check_stop(qcheck);
			step_motor_set_callback(motor, NULL, NULL);
		}
		break;
	case MCS_DONE:
	case MCS_ERROR:
		break;
	default:
		ret = MCS_ERROR;
		break;
	}
	return ret;
}

static  MODBUS_CMD_STATE run_cmd_update_origin(MODBUS_CMD_STATE state, PZTQ_CheckInfo * qcheck)
{
	MODBUS_CMD_STATE ret = state;
	switch(state) {
	case MCS_INIT:
		pztq_check_start(qcheck);
		pzt_q_check_baseline_init(qcheck);
		ret = MCS_BUSY;
		break;
	case MCS_BUSY:
		if (pzt_q_check_baseline(qcheck)) {
			pztq_check_stop(qcheck);
			ret = MCS_DONE;
		}
		break;
	case MCS_DONE:
	case MCS_ERROR:
		break;
	default:
		ret = MCS_ERROR;
		break;
	}
	return ret;
}

static  MODBUS_CMD_STATE run_cmd_update_rc(MODBUS_CMD_STATE state, PZTQ_CheckInfo * qcheck)
{
	MODBUS_CMD_STATE ret = state;
	switch(state) {
	case MCS_INIT:
		pztq_check_start(qcheck);
		pzt_q_check_rc_init(qcheck);
		ret = MCS_BUSY;
		break;
	case MCS_BUSY:
		if (pzt_q_check_rc(qcheck)) {
			pztq_check_stop(qcheck);
			ret = qcheck->rc_runtime.result ? MCS_DONE : MCS_ERROR;
		}
		break;
	case MCS_DONE:
	case MCS_ERROR:
		break;
	default:
		ret = MCS_ERROR;
		break;
	}
	return ret;
}

static  MODBUS_CMD_STATE run_cmd_motor_reset(MODBUS_CMD_STATE state, StepMotor_t *motor)
{
	MODBUS_CMD_STATE ret = state;
	switch(state) {
	case MCS_INIT:
		step_motor_reset_init(motor);
		ret = MCS_BUSY;
		break;
	case MCS_BUSY:
		if (step_motor_reset(motor)) {
			ret = motor->pos_valid == 2 ? MCS_DONE : MCS_ERROR;
		}
		break;
	case MCS_DONE:
	case MCS_ERROR:
		break;
	default:
		ret = MCS_ERROR;
		break;
	}
	return ret;
}

void interface_run(void)
{
	switch(system_state.bits.cmd1) {
	case MC_IDLE:
		system_state.bits.state1 = run_cmd_idle();
		break;
	case MC_MOVE_ABS:
		system_state.bits.state1 = run_cmd_move(system_state.bits.state1,
												&step_motor1,
												m1_regs.pos_set - step_motor1.pos,
												m1_regs.period_set);
		break;
	case MC_MOVE_REL:
		system_state.bits.state1 = run_cmd_move(system_state.bits.state1,
												&step_motor1,
												m1_regs.step_set,
												m1_regs.period_set);
		break;
	case MC_PRESS:
		system_state.bits.state1 = run_cmd_press(system_state.bits.state1,
												&step_motor1,
												&pztq_check1,
												m1_regs.step_set,
												m1_regs.period_set);
		break;
	case MC_UPDATE_ORIGIN:
		system_state.bits.state1 = run_cmd_update_origin(system_state.bits.state1, &pztq_check1);
		break;
	case MC_UPDATE_RC:
		system_state.bits.state1 = run_cmd_update_rc(system_state.bits.state1, &pztq_check1);
		break;
	case MC_MOTOR_RESET:
		system_state.bits.state1 = run_cmd_motor_reset(system_state.bits.state1, &step_motor1);
		break;
	default:
		system_state.bits.state1 = MCS_ERROR;
		break;
	}

	switch(system_state.bits.cmd2) {
	case MC_IDLE:
		system_state.bits.state2 = run_cmd_idle();
		break;
	case MC_MOVE_ABS:
		system_state.bits.state2 = run_cmd_move(system_state.bits.state2,
												&step_motor2,
												m2_regs.pos_set - step_motor2.pos,
												m2_regs.period_set);
		break;
	case MC_MOVE_REL:
		system_state.bits.state2 = run_cmd_move(system_state.bits.state2,
												&step_motor2,
												m2_regs.step_set,
												m2_regs.period_set);
		break;
	case MC_PRESS:
		system_state.bits.state2 = run_cmd_press(system_state.bits.state2,
												&step_motor2,
												&pztq_check2,
												m2_regs.step_set,
												m2_regs.period_set);
		break;
	case MC_UPDATE_ORIGIN:
		system_state.bits.state2 = run_cmd_update_origin(system_state.bits.state2, &pztq_check2);
		break;
	case MC_UPDATE_RC:
		system_state.bits.state2 = run_cmd_update_rc(system_state.bits.state2, &pztq_check2);
		break;
	case MC_MOTOR_RESET:
		system_state.bits.state2 = run_cmd_motor_reset(system_state.bits.state2, &step_motor2);
		break;
	default:
		system_state.bits.state2 = MCS_ERROR;
		break;
	}

	if (system_state.bits.state1 == MCS_DONE || system_state.bits.state1 == MCS_ERROR) {
		if (system_ctrl.bits.cmd1 != system_state.bits.cmd1) {
			system_state.bits.cmd1 = system_ctrl.bits.cmd1;
			system_state.bits.state1 = MCS_INIT;
		}
	}
	if (system_state.bits.state2 == MCS_DONE || system_state.bits.state2 == MCS_ERROR) {
		if (system_ctrl.bits.cmd2 != system_state.bits.cmd2) {
			system_state.bits.cmd2 = system_ctrl.bits.cmd2;
			system_state.bits.state2 = MCS_INIT;
		}
	}

	if (step_motor1.msteps != m1_regs.msteps_set && system_state.bits.state1 >= MCS_DONE) {
		step_motor_set_mstep(&step_motor1, m1_regs.msteps_set);
	}

	if (step_motor2.msteps != m2_regs.msteps_set  && system_state.bits.state2 >= MCS_DONE) {
		step_motor_set_mstep(&step_motor2, m2_regs.msteps_set);
	}


#if 0

	switch(system_state.bits.state1) {
	case MCS_INIT:
			switch(system_state.bits.cmd1) {
			case MC_IDLE:
				system_state.bits.state1 = MCS_DONE;
				break;
			case MC_MOVE_ABS:
				step_motor_set_callback(&step_motor1, NULL, NULL);
				motor_move_config(&step_motor1, m1_regs.pos_set - step_motor1.pos, m1_regs.period_set);
				system_state.bits.state1 = MCS_BUSY;
				break;
			case MC_MOVE_REL:
				step_motor_set_callback(&step_motor1, NULL, NULL);
				motor_move_config(&step_motor1, m1_regs.step_set, m1_regs.period_set);
				system_state.bits.state1 = MCS_BUSY;
				break;
			case MC_PRESS:
				pztq_check_reset(&pztq_check1);
				step_motor_set_callback(&step_motor1, (uint8_t (*)(void *))pztq_check, (void *)&pztq_check1);
				pztq_check_start(&pztq_check1);
				motor_move_config(&step_motor1, m1_regs.step_set, m1_regs.period_set);
				system_state.bits.state1 = MCS_BUSY;
				break;
			case MC_UPDATE_ORIGIN:
				pztq_check_start(&pztq_check1);
				pzt_q_check_baseline_init(&pztq_check1);
				system_state.bits.state1 = MCS_BUSY;
				break;
			case MC_UPDATE_RC:
				pztq_check_start(&pztq_check1);
				pzt_q_check_rc_init(&pztq_check1);
				system_state.bits.state1 = MCS_BUSY;
				break;
			default:
				system_state.bits.state1 = MCS_ERROR;
				break;
			}
		break;
	case MCS_BUSY:
			switch(system_state.bits.cmd1) {
			case MC_IDLE:
				system_state.bits.state1 = MCS_DONE;
				break;
			case MC_MOVE_ABS:
			case MC_MOVE_REL:
				if (step_motor1.step == 0) {
					system_state.bits.state1 = MCS_DONE;
				}
				break;
			case MC_PRESS:
				if (step_motor1.step == 0) {
					system_state.bits.state1 = pztq_check1.dF > pztq_check1.trigger_dF ? MCS_DONE : MCS_ERROR;
				}
				break;
			case MC_UPDATE_ORIGIN:
				if (pzt_q_check_baseline(&pztq_check1)) {
					pztq_check_stop(&pztq_check1);
					system_state.bits.state1 = MCS_DONE;
				}
				break;
			case MC_UPDATE_RC:
				if (pzt_q_check_rc(&pztq_check1)) {
					pztq_check_stop(&pztq_check1);
					system_state.bits.state1 = pztq_check1.rc_runtime.result ? MCS_DONE : MCS_ERROR;
				}
				break;
			default:
				system_state.bits.state1 = MCS_ERROR;
				break;
			}
		break;
	case MCS_DONE:
	case MCS_ERROR:
		if (system_ctrl.bits.cmd1 != system_state.bits.cmd1) {
			system_state.bits.cmd1 = system_ctrl.bits.cmd1;
			system_state.bits.state1 = MCS_INIT;
		}
		break;
	}

	switch(system_state.bits.state2) {
	case MCS_INIT:
		switch(system_state.bits.cmd2) {
			case MC_IDLE:
				system_state.bits.state2 = MCS_DONE;
				break;
			case MC_MOVE_ABS:
				step_motor_set_callback(&step_motor2, NULL, NULL);
				motor_move_config(&step_motor2, m2_regs.pos_set - step_motor2.pos, m2_regs.period_set);
				system_state.bits.state2 = MCS_BUSY;
				break;
			case MC_MOVE_REL:
				step_motor_set_callback(&step_motor2, NULL, NULL);
				motor_move_config(&step_motor2, m2_regs.step_set, m2_regs.period_set);
				system_state.bits.state2 = MCS_BUSY;
				break;
			case MC_PRESS:
				pztq_check_reset(&pztq_check2);
				step_motor_set_callback(&step_motor2, (uint8_t (*)(void *))pztq_check, (void *)&pztq_check2);
				pztq_check_start(&pztq_check2);
				motor_move_config(&step_motor2, m2_regs.step_set, m2_regs.period_set);
				system_state.bits.state2 = MCS_BUSY;
				break;
			case MC_UPDATE_ORIGIN:
				pztq_check_start(&pztq_check2);
				pzt_q_check_baseline_init(&pztq_check2);
				system_state.bits.state2 = MCS_BUSY;
				break;
			case MC_UPDATE_RC:
				pztq_check_start(&pztq_check2);
				pzt_q_check_rc_init(&pztq_check2);
				system_state.bits.state2 = MCS_BUSY;
				break;
			default:
				system_state.bits.state2 = MCS_ERROR;
				break;
			}
		break;
	case MCS_BUSY:
		switch(system_state.bits.cmd2) {
		case MC_IDLE:
			system_state.bits.state2 = MCS_DONE;
			break;
		case MC_MOVE_ABS:
		case MC_MOVE_REL:
			if (step_motor2.step == 0) {
				system_state.bits.state2 = MCS_DONE;
			}
			break;
		case MC_PRESS:
			if (step_motor2.step == 0) {
				system_state.bits.state2 = pztq_check2.dF > pztq_check2.trigger_dF ? MCS_DONE : MCS_ERROR;
			}
			break;
		case MC_UPDATE_ORIGIN:
			if (pzt_q_check_baseline(&pztq_check2)) {
				pztq_check_stop(&pztq_check2);
				system_state.bits.state2 = MCS_DONE;
			}
			break;
		case MC_UPDATE_RC:
			if (pzt_q_check_rc(&pztq_check2)) {
				pztq_check_stop(&pztq_check2);
				system_state.bits.state2 = pztq_check2.rc_runtime.result ? MCS_DONE : MCS_ERROR;
			}
			break;
		default:
			system_state.bits.state2 = MCS_ERROR;
			break;
		}
		break;
	case MCS_DONE:
	case MCS_ERROR:
		if (system_ctrl.bits.cmd2 != system_state.bits.cmd2) {
			system_state.bits.cmd2 = system_ctrl.bits.cmd2;
			system_state.bits.state2 = MCS_INIT;
		}
		break;
	}
#endif
}
#else
static uint8_t interface_buffer[256];
static uint8_t interface_buffer_copy[256];
static uint8_t interface_tx_buffer[256];
static SoftTimer* stimer = NULL;
static void trigger_2ms_callback(void)
{
	HAL_GPIO_TogglePin(TIRGGER_GPIO_Port, TIRGGER_Pin);
}
void interface_start(void)
{
	if (huart2.Instance->SR & USART_SR_RXNE)
			(void)huart2.Instance->DR;
	HAL_UARTEx_ReceiveToIdle_IT(&huart2, interface_buffer, sizeof(interface_buffer));
}

void interface_receive(uint16_t size)
{
	memcpy(interface_buffer_copy, interface_buffer, size);
	interface_start();
}

void interface_send(void* buffer, uint16_t size)
{
	memcpy(interface_tx_buffer, buffer, size);
	HAL_UART_Transmit(&huart2, interface_tx_buffer, size, 100);
}

void press_stop(void)
{
	pzt_check_stop();
	stopSoftTimer(stimer);
	stimer = NULL;
	step_motor_stop(&step_motor1);
}

void interface_run(void)
{
	uint16_t steps = 0;
	uint16_t period = 0;
	if (strncmp(interface_buffer_copy, "up", strlen("up")) == 0) {
		sscanf(interface_buffer_copy, "up %d %d", &period, &steps);
//		pzt_check_start();
//		stopSoftTimer(stimer);
		if (steps < 60) {
			steps = 60;
		} else if (steps > 30000) {
			steps = 30000;
		}
		if (period < 100)
			period = 100;
//		stimer = setSoftTimer(5, trigger_2ms_callback);
		memset(interface_buffer_copy, 0, sizeof(interface_buffer_copy));
		pztq_check_reset(&pztq_check1);
		pztq_check_reset(&pztq_check2);
		step_motor_set_mstep(&step_motor1, 2); // 原来是6 调加减速改为2
		step_motor_set_dir(&step_motor1, MDIR_UP);
		step_motor_set_period_steps(&step_motor1, period, steps);
		step_motor_start(&step_motor1);
	} else if (strncmp(interface_buffer_copy, "down", strlen("down")) == 0) {
		sscanf(interface_buffer_copy, "down %d %d", &period, &steps);
		if (steps < 60) {
			steps = 60;
		} else if (steps > 30000) {
			steps = 30000;
		}
		if (period < 100)
			period = 100;
//		pzt_check_start();
//		stopSoftTimer(stimer);
//		stimer = setSoftTimer(5, trigger_2ms_callback);
		memset(interface_buffer_copy, 0, sizeof(interface_buffer_copy));
		pztq_check_reset(&pztq_check1);
		pztq_check_reset(&pztq_check2);
		step_motor_set_mstep(&step_motor1, 2); // 原来是6 调加减速改为2
		step_motor_set_dir(&step_motor1, MDIR_DOWN);
		step_motor_set_period_steps(&step_motor1, period, steps);
		step_motor_start(&step_motor1);
	} else if (strncmp(interface_buffer_copy, "stop", strlen("stop")) == 0) {
		pzt_check_stop();
//		stopSoftTimer(stimer);
//		stimer = NULL;
		HAL_TIM_Base_Stop_IT(&htim10);
		memset(interface_buffer_copy, 0, sizeof(interface_buffer_copy));
		step_motor_stop(&step_motor1);
	}  else if (strncmp(interface_buffer_copy, "check", strlen("check")) == 0) {
//		pzt_check_start();
////		stopSoftTimer(stimer);
////		stimer = setSoftTimer(5, trigger_2ms_callback);
//
//		HAL_TIM_Base_Start_IT(&htim10);
		pzt_q_check_test();
		memset(interface_buffer_copy, 0, sizeof(interface_buffer_copy));
	}  else if (strncmp(interface_buffer_copy, "press", strlen("press")) == 0) {
//		sscanf(interface_buffer_copy, "press %d", &steps);
//		pzt_set_target(1, steps);
//		pzt_check_start();
//		stopSoftTimer(stimer);
//		stimer = setSoftTimer(5, trigger_2ms_callback);
//		HAL_TIM_Base_Start_IT(&htim10);
		memset(interface_buffer_copy, 0, sizeof(interface_buffer_copy));
		pztq_check_reset(&pztq_check1);
		pztq_check_reset(&pztq_check2);
		step_motor_set_mstep(&step_motor1, 0);
		step_motor_set_dir(&step_motor1, MDIR_DOWN);
		step_motor_set_period_steps(&step_motor1, 1000, 20000);
		step_motor_start(&step_motor1);
	}  else if (strncmp(interface_buffer_copy, "driver", strlen("driver")) == 0) {
		sscanf(interface_buffer_copy, "driver %d", &steps);
		dcdc_set_power(steps);
		memset(interface_buffer_copy, 0, sizeof(interface_buffer_copy));
	}  else if (strncmp(interface_buffer_copy, "setdF", strlen("setdF")) == 0) {
		int dF = 0;
		sscanf(interface_buffer_copy, "setdF %d", &dF);
		pzt_q_set_trigger_dF(&pztq_check1, dF);
		pzt_q_set_trigger_dF(&pztq_check2, dF);
		memset(interface_buffer_copy, 0, sizeof(interface_buffer_copy));
	}

}

#endif




// 测试函数 线性拟合
void linear_fit(int16_t *x, int16_t *y, uint16_t size, float *k, float *b)
{
	int32_t D_xtx;
	int32_t x_sum = 0;
	int32_t x2_sum = 0;
	uint16_t i = 0;
	float k_inner = 0.f, b_inner = 0.f;
	for (i = 0; i < size; i++) {
		x_sum += x[i];
		x2_sum += x[i] * x[i];
	}

	D_xtx = size * x2_sum - x_sum * x_sum;

	for (i = 0; i < size; i++) {
		b_inner +=  (x2_sum - x_sum * x[i]) * y[i];
		k_inner +=  (size * x[i] - x_sum) * y[i];
	}
	*b = b_inner / D_xtx;
	*k = k_inner / D_xtx;
}
