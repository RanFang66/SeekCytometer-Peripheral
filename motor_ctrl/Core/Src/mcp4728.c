/*
 * mcp4728.c
 *
 *  Created on: 2025年11月27日
 *      Author: ranfa
 */

#include "mcp4728.h"
#include "bsp_gpio.h"
#include "cmsis_os2.h"
#include <string.h>

// MCP4728 Instance
static const MCP4728_TypeDef mcp4728 = {
		.SCL_Port = MCP4728_I2C_SCL_GPIO,
		.SCL_Pin = MCP4728_I2C_SCL_PIN,
		.SDA_Port = MCP4728_I2C_SDA_GPIO,
		.SDA_Pin = MCP4728_I2C_SDA_PIN,
		.LDAC_Port = MCP4728_I2C_LDAC_GPIO,
		.LDAC_Pin = MCP4728_I2C_LDAC_PIN,
};


// Thread and message queue
static osThreadId_t mcp4728ThreadHandle = NULL;
static osMessageQueueId_t mcp4728QueueHandle = NULL;

// MCP4728 address
static uint8_t mcp4728Address = MCP4728_DEFAULT_ADDRESS;


// Static function declaration
static void MCP4728_Thread(void *argument);
static MCP4728_Status_t MCP4728_SetChannel_Internal(uint8_t channel, uint16_t value);
static MCP4728_Status_t MCP4728_SetAllChannels_Internal(uint16_t valueA, uint16_t valueB, uint16_t valueC, uint16_t valueD);
static void I2C_Start(void);
static void I2C_Stop(void);
static MCP4728_Status_t I2C_Wait_Ack(void);
static void I2C_Ack(void);
static void I2C_NAck(void);
static void I2C_Send_Byte(uint8_t byte);
static uint8_t I2C_Read_Byte(uint8_t ack);

// us delay based on FreeRTOS tick
static void I2C_Delay(uint32_t us)
{
	if (us < 1000) {
		volatile uint32_t count = us * (SystemCoreClock / 1000000 / 10);
		while (count--);
	} else {
		osDelay(us / 1000);
	}
}

// Initialize MCP4728
void MCP4728_Init()
{
    // Initial state: SCL high, SDA high, LDAC high
    HAL_GPIO_WritePin(mcp4728.SCL_Port, mcp4728.SCL_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(mcp4728.SDA_Port, mcp4728.SDA_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(mcp4728.LDAC_Port, mcp4728.LDAC_Pin, GPIO_PIN_SET);
}

// Create MCP4728 I2C control thread
osThreadId_t MCP4728_Thread_Create(void)
{
    if (mcp4728ThreadHandle != NULL) {
        return mcp4728ThreadHandle;
    }

    // Create message queue (Maximum message number is 16)
    mcp4728QueueHandle = osMessageQueueNew(16, sizeof(MCP4728_Message_t), NULL);
    if (mcp4728QueueHandle == NULL) {
        return NULL;
    }

    // Thread attribute
    const osThreadAttr_t mcp4728Thread_attributes = {
        .name = "MCP4728Thread",
        .stack_size = 1024,
        .priority = (osPriority_t) osPriorityAboveNormal,
    };

    // Create thread
    mcp4728ThreadHandle = osThreadNew(MCP4728_Thread, NULL, &mcp4728Thread_attributes);

    return mcp4728ThreadHandle;
}

// Set DAC value
MCP4728_Status_t MCP4728_Set_Value(MCP4728_Channel_t channel, uint16_t value)
{
    if (mcp4728QueueHandle == NULL) {
        return MCP4728_ERROR;
    }

    MCP4728_Message_t msg = {
        .channel = channel,
        .value = value
    };

    if (osMessageQueuePut(mcp4728QueueHandle, &msg, 0, 0) != osOK) {
        return MCP4728_ERROR;
    }

    return MCP4728_OK;
}

// Set DAC output voltage
MCP4728_Status_t MCP4728_Set_Voltage(MCP4728_Channel_t channel, float voltage, float vref)
{
    if (vref <= 0.0f) {
        return MCP4728_ERROR;
    }

    // Calculate DAC value: value = (voltage / vref) * 4095
    uint16_t value = (uint16_t)((voltage / vref) * 4095.0f);
    if (value > 4095) {
        value = 4095;
    }

    return MCP4728_Set_Value(channel, value);
}

// DAC control thread
static void MCP4728_Thread(void *argument)
{
    MCP4728_Message_t msg;
    MCP4728_Status_t status;

    for (;;) {
        // Wait for message
        if (osMessageQueueGet(mcp4728QueueHandle, &msg, NULL, osWaitForever) == osOK) {
            // Set DAC value according to message received
            switch (msg.channel) {
                case MCP4728_CHANNEL_A:
                    status = MCP4728_SetChannel_Internal(0, msg.value);
                    break;
                case MCP4728_CHANNEL_B:
                    status = MCP4728_SetChannel_Internal(1, msg.value);
                    break;
                case MCP4728_CHANNEL_C:
                    status = MCP4728_SetChannel_Internal(2, msg.value);
                    break;
                case MCP4728_CHANNEL_D:
                    status = MCP4728_SetChannel_Internal(3, msg.value);
                    break;
                case MCP4728_CHANNEL_ALL:
                    status = MCP4728_SetAllChannels_Internal(msg.value, msg.value, msg.value, msg.value);
                    break;
                default:
                    status = MCP4728_ERROR;
                    break;
            }

            // Error handling
            if (status != MCP4728_OK) {
                // To be complete
            }
        }
    }
}

// Set single DAC channel
static MCP4728_Status_t MCP4728_SetChannel_Internal(uint8_t channel, uint16_t value)
{
    // Channel select (A=0, B=1, C=2, D=3)
    uint8_t cmd = MCP4728_CMD_WRITEDAC | ((channel & 0x03) << 1);

    // Limit value
    if (value > 4095) {
        value = 4095;
    }

    // Start I2C
    I2C_Start();

    // Send address + write bit
    I2C_Send_Byte(mcp4728Address << 1);
    if (I2C_Wait_Ack() != MCP4728_OK) {
        I2C_Stop();
        return MCP4728_ERROR;
    }

    // Send command
    I2C_Send_Byte(cmd);
    if (I2C_Wait_Ack() != MCP4728_OK) {
        I2C_Stop();
        return MCP4728_ERROR;
    }

    // Send high 8bit data
    I2C_Send_Byte((value >> 8) & 0x0F);  // MCP4728 is 12bit depth DAC
    if (I2C_Wait_Ack() != MCP4728_OK) {
        I2C_Stop();
        return MCP4728_ERROR;
    }

    // Send low 8bit data
    I2C_Send_Byte(value & 0xFF);
    if (I2C_Wait_Ack() != MCP4728_OK) {
        I2C_Stop();
        return MCP4728_ERROR;
    }

    // Stop I2C
    I2C_Stop();

    // Trigger LDAC to update all channels output
    HAL_GPIO_WritePin(mcp4728.LDAC_Port, mcp4728.LDAC_Pin, GPIO_PIN_RESET);
    I2C_Delay(1);  // At least 500ns pulse width
    HAL_GPIO_WritePin(mcp4728.LDAC_Port, mcp4728.LDAC_Pin, GPIO_PIN_SET);

    return MCP4728_OK;
}

// Set output value in all channels
static MCP4728_Status_t MCP4728_SetAllChannels_Internal(uint16_t valueA, uint16_t valueB, uint16_t valueC, uint16_t valueD)
{
    // Limit value
    if (valueA > 4095) valueA = 4095;
    if (valueB > 4095) valueB = 4095;
    if (valueC > 4095) valueC = 4095;
    if (valueD > 4095) valueD = 4095;

    // Start I2C
    I2C_Start();

    // Send address + write bit
    I2C_Send_Byte(mcp4728Address << 1);
    if (I2C_Wait_Ack() != MCP4728_OK) {
        I2C_Stop();
        return MCP4728_ERROR;
    }

    // Send fast write command
    I2C_Send_Byte(0x40);  // Fast write command
    if (I2C_Wait_Ack() != MCP4728_OK) {
        I2C_Stop();
        return MCP4728_ERROR;
    }

    // Send channel A value
    I2C_Send_Byte((valueA >> 8) & 0x0F);
    if (I2C_Wait_Ack() != MCP4728_OK) {
        I2C_Stop();
        return MCP4728_ERROR;
    }
    I2C_Send_Byte(valueA & 0xFF);
    if (I2C_Wait_Ack() != MCP4728_OK) {
        I2C_Stop();
        return MCP4728_ERROR;
    }

    // Send channel B value
    I2C_Send_Byte((valueB >> 8) & 0x0F);
    if (I2C_Wait_Ack() != MCP4728_OK) {
        I2C_Stop();
        return MCP4728_ERROR;
    }
    I2C_Send_Byte(valueB & 0xFF);
    if (I2C_Wait_Ack() != MCP4728_OK) {
        I2C_Stop();
        return MCP4728_ERROR;
    }

    // Send channel C value
    I2C_Send_Byte((valueC >> 8) & 0x0F);
    if (I2C_Wait_Ack() != MCP4728_OK) {
        I2C_Stop();
        return MCP4728_ERROR;
    }
    I2C_Send_Byte(valueC & 0xFF);
    if (I2C_Wait_Ack() != MCP4728_OK) {
        I2C_Stop();
        return MCP4728_ERROR;
    }

    // Send channel D value
    I2C_Send_Byte((valueD >> 8) & 0x0F);
    if (I2C_Wait_Ack() != MCP4728_OK) {
        I2C_Stop();
        return MCP4728_ERROR;
    }
    I2C_Send_Byte(valueD & 0xFF);
    if (I2C_Wait_Ack() != MCP4728_OK) {
        I2C_Stop();
        return MCP4728_ERROR;
    }

    // Stop I2C
    I2C_Stop();

    // Trigger LDAC to update all channels output
    HAL_GPIO_WritePin(mcp4728.LDAC_Port, mcp4728.LDAC_Pin, GPIO_PIN_RESET);
    I2C_Delay(1);
    HAL_GPIO_WritePin(mcp4728.LDAC_Port, mcp4728.LDAC_Pin, GPIO_PIN_SET);

    return MCP4728_OK;
}

// Generate I2C start signal
static void I2C_Start(void)
{
    // SDA high, SCL high
    HAL_GPIO_WritePin(mcp4728.SDA_Port, mcp4728.SDA_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(mcp4728.SCL_Port, mcp4728.SCL_Pin, GPIO_PIN_SET);
    I2C_Delay(5);

    // SDA low
    HAL_GPIO_WritePin(mcp4728.SDA_Port, mcp4728.SDA_Pin, GPIO_PIN_RESET);
    I2C_Delay(5);

    // SCL low
    HAL_GPIO_WritePin(mcp4728.SCL_Port, mcp4728.SCL_Pin, GPIO_PIN_RESET);
    I2C_Delay(5);
}

// Generate I2C stop signal
static void I2C_Stop(void)
{
    // SDA low, SCL low
    HAL_GPIO_WritePin(mcp4728.SDA_Port, mcp4728.SDA_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(mcp4728.SCL_Port, mcp4728.SCL_Pin, GPIO_PIN_RESET);
    I2C_Delay(5);

    // SCL high
    HAL_GPIO_WritePin(mcp4728.SCL_Port, mcp4728.SCL_Pin, GPIO_PIN_SET);
    I2C_Delay(5);

    // SDA high
    HAL_GPIO_WritePin(mcp4728.SDA_Port, mcp4728.SDA_Pin, GPIO_PIN_SET);
    I2C_Delay(5);
}

// Wait for ACK signal
static MCP4728_Status_t I2C_Wait_Ack(void)
{
    uint16_t timeout = 1000;
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // SDA input mode
    GPIO_InitStruct.Pin = mcp4728.SDA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(mcp4728.SDA_Port, &GPIO_InitStruct);

    // SCL high
    HAL_GPIO_WritePin(mcp4728.SCL_Port, mcp4728.SCL_Pin, GPIO_PIN_SET);
    I2C_Delay(5);

    // Check SDA is low
    while (HAL_GPIO_ReadPin(mcp4728.SDA_Port, mcp4728.SDA_Pin)) {
        timeout--;
        if (timeout == 0) {
            // Set SDA to output mode
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
            HAL_GPIO_Init(mcp4728.SDA_Port, &GPIO_InitStruct);

            // SCL low
            HAL_GPIO_WritePin(mcp4728.SCL_Port, mcp4728.SCL_Pin, GPIO_PIN_RESET);
            I2C_Delay(5);

            return MCP4728_TIMEOUT;
        }
        I2C_Delay(1);
    }

    // SCL low
    HAL_GPIO_WritePin(mcp4728.SCL_Port, mcp4728.SCL_Pin, GPIO_PIN_RESET);
    I2C_Delay(5);

    // set SDA to output mode
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    HAL_GPIO_Init(mcp4728.SDA_Port, &GPIO_InitStruct);

    return MCP4728_OK;
}

// Generate ACK signal
static void I2C_Ack(void)
{
    // SDA low
    HAL_GPIO_WritePin(mcp4728.SDA_Port, mcp4728.SDA_Pin, GPIO_PIN_RESET);
    I2C_Delay(5);

    // SCL high
    HAL_GPIO_WritePin(mcp4728.SCL_Port, mcp4728.SCL_Pin, GPIO_PIN_SET);
    I2C_Delay(5);

    // SCL low
    HAL_GPIO_WritePin(mcp4728.SCL_Port, mcp4728.SCL_Pin, GPIO_PIN_RESET);
    I2C_Delay(5);

    // SDA high
    HAL_GPIO_WritePin(mcp4728.SDA_Port, mcp4728.SDA_Pin, GPIO_PIN_SET);
    I2C_Delay(5);
}

// Generate NACK signal
static void I2C_NAck(void)
{
    // SDA high
    HAL_GPIO_WritePin(mcp4728.SDA_Port, mcp4728.SDA_Pin, GPIO_PIN_SET);
    I2C_Delay(5);

    // SCL high
    HAL_GPIO_WritePin(mcp4728.SCL_Port, mcp4728.SCL_Pin, GPIO_PIN_SET);
    I2C_Delay(5);

    // SCL low
    HAL_GPIO_WritePin(mcp4728.SCL_Port, mcp4728.SCL_Pin, GPIO_PIN_RESET);
    I2C_Delay(5);
}

// Send one byte data
static void I2C_Send_Byte(uint8_t byte)
{
    uint8_t i;

    for (i = 0; i < 8; i++) {
        if (byte & 0x80)
            HAL_GPIO_WritePin(mcp4728.SDA_Port, mcp4728.SDA_Pin, GPIO_PIN_SET);
        else
            HAL_GPIO_WritePin(mcp4728.SDA_Port, mcp4728.SDA_Pin, GPIO_PIN_RESET);

        byte <<= 1;
        I2C_Delay(5);

        // SCL high
        HAL_GPIO_WritePin(mcp4728.SCL_Port, mcp4728.SCL_Pin, GPIO_PIN_SET);
        I2C_Delay(5);

        // SCL low
        HAL_GPIO_WritePin(mcp4728.SCL_Port, mcp4728.SCL_Pin, GPIO_PIN_RESET);
        I2C_Delay(5);
    }
}

// Read one byte data
static uint8_t I2C_Read_Byte(uint8_t ack)
{
    uint8_t i, byte = 0;
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Set SDA to input mode
    GPIO_InitStruct.Pin = mcp4728.SDA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(mcp4728.SDA_Port, &GPIO_InitStruct);

    for (i = 0; i < 8; i++) {
        byte <<= 1;

        // SCL high
        HAL_GPIO_WritePin(mcp4728.SCL_Port, mcp4728.SCL_Pin, GPIO_PIN_SET);
        I2C_Delay(5);

        if (HAL_GPIO_ReadPin(mcp4728.SDA_Port, mcp4728.SDA_Pin))
            byte |= 0x01;

        // SCL low
        HAL_GPIO_WritePin(mcp4728.SCL_Port, mcp4728.SCL_Pin, GPIO_PIN_RESET);
        I2C_Delay(5);
    }

    // Set SDA to output mode
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    HAL_GPIO_Init(mcp4728.SDA_Port, &GPIO_InitStruct);

    // Send ACK or NACK signal
    if (ack)
        I2C_Ack();
    else
        I2C_NAck();

    return byte;
}

