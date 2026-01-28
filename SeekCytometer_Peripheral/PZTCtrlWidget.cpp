#include "PZTCtrlWidget.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include "ModbusRegistersTable.h"
#include "ModbusMaster.h"

#define MOTOR1_CMD(cmd)   (uint16_t)cmd
#define MOTOR2_CMD(cmd)  ((uint16_t)cmd << 8)

#define MOTOR_CMD(id, cmd) ((uint16_t)cmd << (8 * id))




PZTCtrlWidget::PZTCtrlWidget(const QString &tilte, QWidget *parent)
    : QDockWidget{tilte, parent}
{
    initDockWidget();
}

void PZTCtrlWidget::updateStatus(const QVector<uint16_t> &regs)
{
    motor1->updateStatus(regs);
    motor2->updateStatus(regs);
}

PZTMotorWidget::PZTMotorWidget(const QString &title, int id, QWidget *parent)
    : QGroupBox(title, parent), m_id(id)
{
    initWidget();

    connect(btnMotorUp, &ArrowButton::clicked, this, &PZTMotorWidget::onBtnMotorUpClicked);
    connect(btnMotorDown, &ArrowButton::clicked, this, &PZTMotorWidget::onBtnMotorDownClicked);
    connect(btnMotorPress, &QPushButton::clicked, this, &PZTMotorWidget::onBtnMotorPressClicked);
    connect(btnMotorRunToPos, &QPushButton::clicked, this, &PZTMotorWidget::onBtnMotorRunToPos);
    connect(btnMotorStop, &QPushButton::clicked, this, &PZTMotorWidget::onBtnMotorStopClicked);
    connect(btnMotorUpdateOrigin, &QPushButton::clicked, this, &PZTMotorWidget::onBtnMotorUpdateOriginClicked);
    connect(btnMotorUpdateRC, &QPushButton::clicked, this, &PZTMotorWidget::onBtnMotorUpdateRCClicked);
    connect(spinMotorPeriod, &QSpinBox::editingFinished, this, &PZTMotorWidget::onSpinMotorPeriodSet);
    connect(spinMotorSpeed, &QSpinBox::editingFinished, this, &PZTMotorWidget::onSpinMotorSpeedSet);
    connect(spinMotorPosTarget, &QSpinBox::editingFinished, this, &PZTMotorWidget::onSpinMotorTargetPosSet);
    connect(spinMotorTrigger, &QSpinBox::editingFinished, this, &PZTMotorWidget::onSpinTriggerSet);
    connect(spinMotorSteps, &QSpinBox::editingFinished, this, &PZTMotorWidget::onSpinMotorStepsSet);
    connect(btnMotorReset, &QPushButton::clicked, this, &PZTMotorWidget::onMotorResetClicked);
}

void PZTMotorWidget::updateStatus(const QVector<uint16_t> &regs)
{
    if (m_id == 0) {
        m_status = (regs[0] >> 6) & 0x0003;
        m_pos = (int)(((uint32_t)regs[1] << 16) + regs[2]);
        m_adcBaseLine = regs[5];
        m_adcThreshold = regs[6];
        m_rc = regs[7];
    } else if (m_id == 1) {
        m_status = (regs[0] >> 14) & 0x0003;
        m_pos = (int)(((uint32_t)regs[3] << 16) + regs[4]);
        m_adcBaseLine = regs[8];
        m_adcThreshold = regs[9];
        m_rc = regs[10];
    }
    if (m_status == 0 || m_status == 2) {
        statusLightMotor->setState(StatusLight::State::IDLE);
    } else if (m_status == 1) {
        statusLightMotor->setState(StatusLight::State::RUNNING);
    } else if (m_status == 3) {
        statusLightMotor->setState(StatusLight::State::FAULT);
    }

    lblMotorPos->setText(QString("Pos: %1").arg(m_pos));
    lblAdcInfo->setText(QString("Adc Baseline :%1, thresh: %2").arg(m_adcBaseLine).arg(m_adcThreshold));
    lblRCInfo->setText(QString("RC: %1").arg(m_rc));

}

void PZTMotorWidget::onBtnMotorUpClicked()
{
    int steps = -spinMotorSteps->value();
    uint16_t cmd = MOTOR_CMD(m_id, PZT_CMD_MOVE_REL);

    QVector<uint16_t> stepsBytes(2, 0);
    stepsBytes[0] = (uint16_t)((steps >> 16) & 0x00FF);
    stepsBytes[1] = (uint16_t)(steps & 0x00FF);

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, PZT_MOTOR_STEP_SET(m_id), stepsBytes);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, PZT_CW, 0);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, PZT_CW, cmd);
}

void PZTMotorWidget::onBtnMotorDownClicked()
{
    int steps = spinMotorSteps->value();
    uint16_t cmd = MOTOR_CMD(m_id, PZT_CMD_MOVE_REL);
    QVector<uint16_t> stepsBytes(2, 0);
    stepsBytes[0] = (uint16_t)((steps >> 16) & 0x00FF);
    stepsBytes[1] = (uint16_t)(steps & 0x00FF);

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, PZT_MOTOR_STEP_SET(m_id), stepsBytes);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, PZT_CW, 0);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, PZT_CW, cmd);
}

void PZTMotorWidget::onBtnMotorRunToPos()
{
    int pos = spinMotorSteps->value();
    uint16_t cmd = MOTOR_CMD(m_id, PZT_CMD_MOVE_ABS);

    QVector<uint16_t> posBytes(2, 0);
    posBytes[0] = (uint16_t)((pos >> 16) & 0x00FF);
    posBytes[1] = (uint16_t)(pos & 0x00FF);
    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, PZT_MOTOR_POS_SET(m_id), posBytes);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, PZT_CW, 0);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, PZT_CW, cmd);
}

void PZTMotorWidget::onBtnMotorPressClicked()
{
    int steps = spinMotorSteps->value();
    uint16_t cmd = MOTOR_CMD(m_id, PZT_CMD_PRESS);
    int triggerDf = spinMotorTrigger->value();

    QVector<uint16_t> stepsBytes(2, 0);
    stepsBytes[0] = (uint16_t)((steps >> 16) & 0x00FF);
    stepsBytes[1] = (uint16_t)(steps & 0x00FF);
    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, PZT_MOTOR_STEP_SET(m_id), stepsBytes);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, PZT_MOTOR_TRIGGER_SET(m_id), (uint16_t)triggerDf);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, PZT_CW, 0);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, PZT_CW, cmd);
}

void PZTMotorWidget::onMotorResetClicked()
{
    uint16_t cmd = MOTOR_CMD(m_id, PZT_CMD_RESET);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, PZT_CW, 0);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, PZT_CW, cmd);
}

void PZTMotorWidget::onBtnMotorStopClicked()
{
    uint16_t cmd = MOTOR_CMD(m_id, PZT_CMD_IDLE);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, PZT_CW, cmd);
}

void PZTMotorWidget::onBtnMotorUpdateOriginClicked()
{
    uint16_t cmd = MOTOR_CMD(m_id, PZT_CMD_UPDATE_ORGIN);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, PZT_CW, 0);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, PZT_CW, cmd);
}

void PZTMotorWidget::onBtnMotorUpdateRCClicked()
{
    uint16_t cmd = MOTOR_CMD(m_id, PZT_CMD_UPDATE_RC);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, PZT_CW, 0);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, PZT_CW, cmd);
}

void PZTMotorWidget::onSpinMotorStepsSet()
{
    int steps = spinMotorSteps->value();
    QVector<uint16_t> stepsBytes(2, 0);
    stepsBytes[0] = (uint16_t)((steps >> 16) & 0x00FF);
    stepsBytes[1] = (uint16_t)(steps & 0x00FF);

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, PZT_MOTOR_STEP_SET(m_id), stepsBytes);
}

void PZTMotorWidget::onSpinMotorTargetPosSet()
{
    int pos = spinMotorSteps->value();


    QVector<uint16_t> posBytes(2, 0);
    posBytes[0] = (uint16_t)((pos >> 16) & 0x00FF);
    posBytes[1] = (uint16_t)(pos & 0x00FF);

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, PZT_MOTOR_POS_SET(m_id), posBytes);
}

void PZTMotorWidget::onSpinMotorSpeedSet()
{
    int speed = spinMotorSpeed->value();
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, PZT_MOTOR_SPEED_SET(m_id), (uint16_t)speed);
}

void PZTMotorWidget::onSpinMotorPeriodSet()
{
    uint16_t period = spinMotorPeriod->value();
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, PZT_MOTOR_PERIOD_SET(m_id), period);
}

void PZTMotorWidget::onSpinTriggerSet()
{
    int triggerDf = spinMotorTrigger->value();
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, PZT_MOTOR_TRIGGER_SET(m_id), (uint16_t)triggerDf);
}





void PZTCtrlWidget::initDockWidget()
{
    QHBoxLayout *mainLayout = new QHBoxLayout();
    motor1 = new PZTMotorWidget("Motor-1", 0,  this);
    motor2 = new PZTMotorWidget("Motor-2", 1, this);
    mainLayout->addWidget(motor1);
    mainLayout->addWidget(motor2);
    QWidget *mainWidget = new QWidget(this);
    mainWidget->setLayout(mainLayout);
    setWidget(mainWidget);
}

void PZTMotorWidget::initWidget()
{
    btnMotorDown = new ArrowButton(ArrowButton::Direction::Down, this);
    btnMotorUp = new ArrowButton(ArrowButton::Direction::Up, this);
    btnMotorReset = new QPushButton("Reset", this);
    btnMotorStop = new QPushButton("Stop", this);
    btnMotorPress = new QPushButton("Press", this);
    btnMotorRunToPos = new QPushButton("RunToPos", this);
    btnMotorUpdateOrigin = new QPushButton("Update Origin", this);
    btnMotorUpdateRC = new QPushButton("Update RC", this);


    spinMotorSteps = new QSpinBox(this);
    spinMotorPeriod = new QSpinBox(this);
    spinMotorPosTarget = new QSpinBox(this);
    spinMotorSpeed = new QSpinBox(this);
    spinMotorTrigger = new QSpinBox(this);




    statusLightMotor = new StatusLight(this);
    lblMotorPos = new QLabel("0", this);
    lblAdcInfo = new QLabel("Adc Baseline :0, thresh: 0", this);
    lblRCInfo = new QLabel("RC: 0", this);

    spinMotorPeriod->setRange(100, 10000);
    spinMotorPeriod->setValue(1000);
    spinMotorSpeed->setRange(0, 8);
    spinMotorSpeed->setValue(2);   // press: 2;
    spinMotorTrigger->setRange(0, 4095);
    spinMotorTrigger->setValue(200);
    spinMotorSteps->setRange(1, 40000);
    spinMotorSteps->setValue(1000);

    auto addSpin = [&](QVBoxLayout *lay, const QString &prompt, QSpinBox *spinbox) {
        QLabel *lblPrompt = new QLabel(prompt, this);
        QHBoxLayout *layout = new QHBoxLayout();
        layout->addWidget(lblPrompt, 0, Qt::AlignCenter);
        layout->addWidget(spinbox, 0, Qt::AlignCenter);
        lay->addLayout(layout);
    };

    auto addSpinButton = [&](QVBoxLayout *lay, QPushButton *button, QSpinBox *spinbox) {
        QHBoxLayout *layout = new QHBoxLayout();
        layout->addWidget(button, 0, Qt::AlignCenter);
        layout->addWidget(spinbox, 0, Qt::AlignCenter);
        lay->addLayout(layout);
    };

    QVBoxLayout *vLayout1 = new QVBoxLayout();
    vLayout1->addWidget(statusLightMotor, 0, Qt::AlignCenter);
    vLayout1->addWidget(lblAdcInfo, 0, Qt::AlignCenter);
    vLayout1->addWidget(lblRCInfo, 0, Qt::AlignCenter);
    vLayout1->addWidget(btnMotorReset);
    vLayout1->addWidget(btnMotorStop);
    addSpin(vLayout1, "Press Trigger Set:", spinMotorTrigger);

    vLayout1->addWidget(btnMotorPress);
    vLayout1->addWidget(btnMotorUpdateOrigin);
    vLayout1->addWidget(btnMotorUpdateRC);
    addSpin(vLayout1, "Period Set:", spinMotorPeriod);
    addSpin(vLayout1, "Speed Set:", spinMotorSpeed);
    addSpinButton(vLayout1, btnMotorRunToPos, spinMotorPosTarget);

    vLayout1->addWidget(btnMotorUp, 0, Qt::AlignCenter);
    vLayout1->addWidget(lblMotorPos, 0, Qt::AlignCenter);
    addSpin(vLayout1, "Steps Set", spinMotorSteps);
    vLayout1->addWidget(btnMotorDown, 0, Qt::AlignCenter);

    setLayout(vLayout1);

}
