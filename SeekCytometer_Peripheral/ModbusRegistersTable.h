#ifndef MODBUSREGISTERSTABLE_H
#define MODBUSREGISTERSTABLE_H

#define SLAVE_ADDR  0x11


#define ADC_COMM_REG_START          (40001)
#define ADC_COMM_REG_COUNT          (100)
#define MOTOR_CTRL_REG_START        (ADC_COMM_REG_START + ADC_COMM_REG_COUNT)
#define MOTOR_CTRL_REG_COUNT        (100)
#define MFC_REG_START               (MOTOR_CTRL_REG_START + MOTOR_CTRL_REG_COUNT)
#define MFC_REG_COUNT               (100)
#define PZT_CTRL_REG_START          (MFC_REG_START + MFC_REG_COUNT)
#define PZT_CTRL_REG_COUNT          (100)


#define AD_REG_ADDR(INDEX)          (ADC_COMM_REG_START + INDEX)
#define MOTOR_REG_ADDR(INDEX)       (MOTOR_CTRL_REG_START + INDEX)
#define MFC_REG_ADDR(INDEX)         (MFC_REG_START + INDEX)
#define PZT_REG_ADDR(INDEX)         (PZT_CTRL_REG_START + INDEX)


#define AD_COMM_CW                  AD_REG_ADDR(0)
#define AD_SAMPLE_CMD               AD_REG_ADDR(6)
#define AD_SAMPLE_EN_CH             AD_REG_ADDR(7)
#define AD_SAMPLE_GAIN_SET_1        AD_REG_ADDR(8)
#define AD_SAMPLE_REF_SET_1         AD_REG_ADDR(16)




#define MOTOR_CTRL_CW               MOTOR_REG_ADDR(0)
#define MOTOR_CTRL_COVER_CMD        MOTOR_REG_ADDR(8)
#define MOTOR_CTRL_SEAL_CMD         MOTOR_REG_ADDR(9)

#define MOTOR_CTRL_CHURN_CMD        MOTOR_REG_ADDR(13)
#define MOTOR_CTRL_CHURN_DATA       MOTOR_REG_ADDR(14)
#define MOTOR_CTRL_MOTOR_CMD        MOTOR_REG_ADDR(15)
#define MOTOR_CTRL_MOTOR_DATA       MOTOR_REG_ADDR(16)
#define MOTOR_CTRL_LASER_CMD        MOTOR_REG_ADDR(18)
#define MOTOR_CTRL_LASER_DATA       MOTOR_REG_ADDR(19)
#define MOTOR_CTRL_LED_CMD          MOTOR_REG_ADDR(20)
#define MOTOR_CTRL_LED_DATA         MOTOR_REG_ADDR(21)
#define MOTOR_CTRL_TEMP_CMD         MOTOR_REG_ADDR(22)
#define MOTOR_CTRL_TEMP_DATA        MOTOR_REG_ADDR(23)
#define MOTOR_CTRL_FAN_CH           MOTOR_REG_ADDR(24)
#define MOTOR_CTRL_FAN_SPEED        MOTOR_REG_ADDR(25)
#define MOTOR_CTRL_FAN_1_SPEED      MOTOR_REG_ADDR(26)
#define MOTOR_CTRL_FAN_2_SPEED      MOTOR_REG_ADDR(27)
#define MOTOR_CTRL_FAN_3_SPEED      MOTOR_REG_ADDR(28)
#define MOTOR_CTRL_FAN_4_SPEED      MOTOR_REG_ADDR(29)


#define MOTOR_CTRL_TEMP_STATUS          MOTOR_REG_ADDR(47)
#define MOTOR_CTRL_CURRENT_TEMP         MOTOR_REG_ADDR(48)

#define MOTOR_CTRL_COVER_STATUS         MOTOR_REG_ADDR(49)
#define MOTOR_CTRL_SEAL_STATUS          MOTOR_REG_ADDR(50)
#define MOTOR_CTRL_CHURN_STATUS         MOTOR_REG_ADDR(51)

#define MOTOR_CTRL_MOTOR_X_STATUS       MOTOR_REG_ADDR(52)
#define MOTOR_CTRL_MOTOR_Y_STATUS       MOTOR_REG_ADDR(53)
#define MOTOR_CTRL_MOTOR_Z_STATUS       MOTOR_REG_ADDR(54)
#define MOTOR_CTRL_MOTOR_LIMIT          MOTOR_REG_ADDR(55)
#define MOTOR_CTRL_X_POS                MOTOR_REG_ADDR(56)
#define MOTOR_CTRL_Y_POS                MOTOR_REG_ADDR(58)
#define MOTOR_CTRL_Z_POS                MOTOR_REG_ADDR(60)
#define MOTOR_CTRL_LASER1_STATUS        MOTOR_REG_ADDR(62)
#define MOTOR_CTRL_LASER1_INTENSITY     MOTOR_REG_ADDR(63)
#define MOTOR_CTRL_LASER2_STATUS        MOTOR_REG_ADDR(64)
#define MOTOR_CTRL_LASER2_INTENSITY     MOTOR_REG_ADDR(65)
#define MOTOR_CTRL_LED_STATUS           MOTOR_REG_ADDR(66)
#define MOTOR_CTRL_LED_INTENSITY        MOTOR_REG_ADDR(67)


#define MFC_CW                          MFC_REG_ADDR(0)
#define MFC_PRESS_CMD                   MFC_REG_ADDR(8)
#define MFC_PRESS_EN_CAHNNEL            MFC_REG_ADDR(9)
#define MFC_PRESS_SET_CH1               MFC_REG_ADDR(10)
#define MFC_PRESS_SET_CH2               MFC_REG_ADDR(11)
#define MFC_PRESS_SET_CH3               MFC_REG_ADDR(12)
#define MFC_PRESS_SET_CH4               MFC_REG_ADDR(13)
#define MFC_PRESS_SET_CH5               MFC_REG_ADDR(14)


#define MFC_SOL_CMD                     MFC_REG_ADDR(16)
#define MFC_SOL_CMD_CH                  MFC_REG_ADDR(17)
#define MFC_PROPO_CMD                   MFC_REG_ADDR(18)
#define MFC_PROPO_CMD_CH                MFC_REG_ADDR(19)
#define MFC_PROPO_CMD_DATA              MFC_REG_ADDR(20)
#define MFC_KP_SET                      MFC_REG_ADDR(21)
#define MFC_KI_SET                      MFC_REG_ADDR(22)
#define MFC_FF_SET                      MFC_REG_ADDR(23)



#define MFC_PRESS_CTRL_STATUS           MFC_REG_ADDR(54)
#define MFC_SOL_CTRL_STATUS             MFC_REG_ADDR(55)
#define MFC_INPUT_PRESS                 MFC_REG_ADDR(56)
#define MFC_PRESS_CH1                   MFC_REG_ADDR(57)
#define MFC_PRESS_CH2                   MFC_REG_ADDR(58)
#define MFC_PRESS_CH3                   MFC_REG_ADDR(59)
#define MFC_PRESS_CH4                   MFC_REG_ADDR(60)
#define MFC_PRESS_CH5                   MFC_REG_ADDR(61)
#define MFC_PROPO_VAL_CH1               MFC_REG_ADDR(62)
#define MFC_PROPO_VAL_CH2               MFC_REG_ADDR(63)
#define MFC_PROPO_VAL_CH3               MFC_REG_ADDR(64)
#define MFC_PROPO_VAL_CH4               MFC_REG_ADDR(65)
#define MFC_PROPO_VAL_CH5               MFC_REG_ADDR(66)
#define MFC_PRESS_TARGET_CH1            MFC_REG_ADDR(67)
#define MFC_PRESS_TARGET_CH2            MFC_REG_ADDR(68)
#define MFC_PRESS_TARGET_CH3            MFC_REG_ADDR(69)
#define MFC_PRESS_TARGET_CH4            MFC_REG_ADDR(70)
#define MFC_PRESS_TARGET_CH5            MFC_REG_ADDR(71)
#define MFC_KP_CH1                      MFC_REG_ADDR(72)

#define PZT_CW                          PZT_REG_ADDR(0)
#define PZT_MOTOR1_SPEED_SET            PZT_REG_ADDR(1)
#define PZT_MOTOR2_SPEED_SET            PZT_REG_ADDR(2)
#define PZT_MOTOR1_PERIOD_SET           PZT_REG_ADDR(3)
#define PZT_MOTOR2_PERIOD_SET           PZT_REG_ADDR(4)
#define PZT_MOTOR1_POS_SET_HI           PZT_REG_ADDR(5)
#define PZT_MOTOR1_POS_SET_LO           PZT_REG_ADDR(6)
#define PZT_MOTOR1_STEP_SET_HI          PZT_REG_ADDR(7)
#define PZT_MOTOR1_STEP_SET_LO          PZT_REG_ADDR(8)
#define PZT_MOTOR1_TRIGGER_SET          PZT_REG_ADDR(9)
#define PZT_MOTOT2_POS_SET_HI           PZT_REG_ADDR(10)
#define PZT_MOTOR2_POS_SET_LO           PZT_REG_ADDR(11)
#define PZT_MOTOR2_STEP_SET_HI          PZT_REG_ADDR(12)
#define PZT_MOTOR2_STEP_SET_LO          PZT_REG_ADDR(13)
#define PZT_MOTOR2_TRIGGER_SET          PZT_REG_ADDR(14)


#define PZT_MOTOR_SPEED_SET(i)          (PZT_MOTOR1_SPEED_SET+i)
#define PZT_MOTOR_PERIOD_SET(i)         (PZT_MOTOR1_PERIOD_SET+i)
#define PZT_MOTOR_POS_SET(i)            (PZT_MOTOR1_POS_SET_HI + i*4)
#define PZT_MOTOR_STEP_SET(i)           (PZT_MOTOR1_STEP_SET_HI + i*4)
#define PZT_MOTOR_TRIGGER_SET(i)        (PZT_MOTOR1_TRIGGER_SET + i*4)

#define PZT_STATUS_WORD                 PZT_REG_ADDR(15)
#define PZT_MOTOR1_POS_HI               PZT_REG_ADDR(16)
#define PZT_MOTOR1_POS_LO               PZT_REG_ADDR(17)
#define PZT_MOTOR2_POS_HI               PZT_REG_ADDR(18)
#define PZT_MOTOR2_POS_LO               PZT_REG_ADDR(19)


#define CW_COVER_BIT                    0x0001U
#define CW_SEAL_BIT                     0x0002U
#define CW_CHURN_BIT                    0x0004U
#define CW_TEMP_BIT                     0x0008U
#define CW_STEPPER_MOTOR_BIT            0x0010U
#define CW_LASER_BIT                    0x0020U
#define CW_LED_BIT                      0x0040U


#define MOTOR_X                         0x00U
#define MOTOR_Y                         0x01U
#define MOTOR_Z                         0x02U

#define MOTOR_CMD_STOP                  0x00U
#define MOTOR_CMD_RUN_STEPS             0x01U
#define MOTOR_CMD_RUN_POS               0x02U
#define MOTOR_CMD_RETURN_HOME           0x03U
#define MOTOR_CMD_RESET                 0x04U

#define MFC_CW_PRESS_BIT                0x0001U
#define MFC_CW_SOLE_BIT                 0x0002U
#define MFC_CW_PROPO_BIT                0x0004U

#define COVER_CMD_OPEN      0x0001
#define COVER_CMD_CLOSE     0x0002
#define SEAL_CMD_PUSH       0x0001
#define SEAL_CMD_RELEASE    0x0002
#define CHURN_CMD_STOP      0x0000
#define CHURN_CMD_RUN_CW    0x0001
#define CHURN_CMD_RUN_CCW   0x0002

#define TEMP_CMD_STOP           0x0000
#define TEMP_CMD_START          0x0001
#define TEMP_CMD_SET_TARGET     0x0002
#define TEMP_CMD_FAN_SET        0x0003
#define TEMP_CMD_FAN_ENABLE     0x0004
#define TEMP_CMD_FAN_DISABLE    0x0005
#define TEMP_CMD_FAN_SET_SPEED  0x0006
#define TEMP_CMD_RESET          0x0007

#define LASER_1_ID              0x0000U
#define LASER_2_ID              0x0001U

#define LASER_CMD_SWITCH_OFF    0X0000
#define LASER_CMD_SWITCH_ON     0x0001
#define LASER_CMD_SET_INTENSITY 0x0002

#define LED_CMD_SWITCH_OFF    0X0000
#define LED_CMD_SWITCH_ON     0x0001
#define LED_CMD_SET_INTENSITY 0x0002

#define MFC_PRESS_CTRL_STOP         0x0000
#define MFC_PRESS_CTRL_START        0x0001
#define MFC_PRESS_CTRL_SET_TARGET   0x0002
#define MFC_PRESS_CTRL_RESET        0x0003
#define MFC_PRESS_CTRL_SET_PI       0x0004

#define SOL_CTRL_CLOSE 		0x0000
#define SOL_CTRL_OPEN		0x0001

#define PROPO_CTRL_CLOSE 	0x0000
#define PROPO_CTRL_OPEN 	0x0001


#define SAMPLE_RESET_GAIN_REF   0x0000
#define SAMPLE_UPDATE_GAIN      0x0001
#define SAMPLE_UPDATE_REF       0x0002
#define SAMPLE_UPDATE_GAIN_REF  0x0003

#endif // MODBUSREGISTERSTABLE_H
