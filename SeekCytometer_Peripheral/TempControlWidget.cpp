#include "TempControlWidget.h"
#include <QGridLayout>
#include <QGroupBox>
#include "ModbusMaster.h"
#include "ModbusRegistersTable.h"

TempControlWidget::TempControlWidget(const QString &tilte, QWidget *parent)
    : QDockWidget{tilte, parent}
{
    initTempControlWidget();
}

void TempControlWidget::updateStatus(const QVector<uint16_t> &regs)
{
    if (regs.length() < 2) {
        qDebug() << "Wrong status registers number! Need 2, but actually " << regs.length();
        return;
    }
    tempCtrlStatus = regs[0];
    currentTemp = (float)regs[1] / 10.0;
    if (tempCtrlStatus == TEMP_CTRL_IDLE) {
        lblTempControlStatus->setState(StatusLight::State::IDLE);
    } else if (tempCtrlStatus == TEMP_CTRL_RUNNING) {
        lblTempControlStatus->setState(StatusLight::State::RUNNING);
    } else {
        lblTempControlStatus->setState(StatusLight::State::FAULT);
    }

    lblCurrentTemp->setText(QString("%1 ℃").arg(currentTemp));
}

void TempControlWidget::initTempControlWidget()
{
    lblTempControlStatus = new StatusLight(this);
    lblTempControlStatus->setState(StatusLight::State::IDLE);
    spinTargetTemp = new QDoubleSpinBox(this);
    spinTargetTemp->setRange(0.0, 25.0);
    spinTargetTemp->setDecimals(1);
    spinTargetTemp->setSingleStep(0.1);
    spinTargetTemp->setValue(4.0);
    btnTempControl = new ToggleRunButton(this);
    lblCurrentTemp = new QLabel("25 ℃");
    QGridLayout *tempLayout = new QGridLayout();

    tempLayout->addWidget(new QLabel(tr("Target Temp(℃)"), this), 0, 0, Qt::AlignCenter);
    tempLayout->addWidget(spinTargetTemp, 0, 1, Qt::AlignCenter);
    tempLayout->addWidget(new QLabel(tr("Current Temp(℃)"), this), 1, 0, Qt::AlignCenter);
    tempLayout->addWidget(lblCurrentTemp, 1, 1, Qt::AlignCenter);
    tempLayout->addWidget(btnTempControl, 0, 2, 2, 1, Qt::AlignCenter);
    tempLayout->addWidget(lblTempControlStatus, 0, 3, 2, 1, Qt::AlignCenter);


    connect(btnTempControl, &ToggleRunButton::toggled, this, &TempControlWidget::onTempControlToggled);
    connect(spinTargetTemp, &QDoubleSpinBox::valueChanged, this, &TempControlWidget::onTempTargetChanged);

    QGroupBox *groupTemp = new QGroupBox(tr("Temp Control"), this);
    groupTemp->setLayout(tempLayout);


    QGridLayout *fanLayout = new QGridLayout();
    fanLayout->addWidget(new QLabel(tr("Fan ID"), this), 0, 0, Qt::AlignCenter);
    fanLayout->addWidget(new QLabel(tr("Enable"), this), 0, 1, Qt::AlignCenter);
    fanLayout->addWidget(new QLabel(tr("Fan Speed"), this), 0, 2, Qt::AlignCenter);
    for (int i = 0; i < FAN_NUM; i++) {
        chkFans[i] = new ToggleSwitch(this);
        spinFanSpeed[i] = new QSpinBox(this);
        spinFanSpeed[i]->setRange(0, 100);
        spinFanSpeed[i]->setValue(0);
        fanLayout->addWidget(new QLabel(QString("Fan-%1").arg(i+1), this), i+1, 0, Qt::AlignCenter);
        fanLayout->addWidget(chkFans[i], i+1, 1);
        fanLayout->addWidget(spinFanSpeed[i], i+1, 2);

        connect(chkFans[i], &ToggleSwitch::toggled, this, [=](bool checked) {
            onFanChToggled(i, checked);
        });

        connect(spinFanSpeed[i], &QSpinBox::valueChanged, this, [=](int value) {
            onFanSpeedChanged(i, value);
        });
    }
    btnFanEnable = new ToggleRunButton(this);
    fanLayout->addWidget(btnFanEnable, 1, 3, 4, 1, Qt::AlignCenter);
    connect(btnFanEnable, &ToggleRunButton::toggled, this, &TempControlWidget::onFanControlToggled);

    QGroupBox *groupFan = new QGroupBox(tr("Fan Control"), this);
    groupFan->setLayout(fanLayout);

    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->addWidget(groupTemp, 1);
    mainLayout->addWidget(groupFan, 2);
    mainWidget->setLayout(mainLayout);
    setWidget(mainWidget);
}

void TempControlWidget::onTempControlToggled(bool checked)
{
    double val = spinTargetTemp->value();
    uint16_t tempTarget = val * 10;


    if (checked) {
        const QVector<uint16_t> tempCmd = {TEMP_CMD_START, tempTarget};
        ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_TEMP_CMD, tempCmd);
        ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
        ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_TEMP_BIT);
    } else {
        ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_TEMP_CMD, TEMP_CMD_STOP);
        ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
        ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_TEMP_BIT);
    }
}

void TempControlWidget::onTempTargetChanged(double value)
{
    uint16_t tempTarget = value * 10;

    const QVector<uint16_t> tempCmd = {TEMP_CMD_SET_TARGET, tempTarget};
    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_TEMP_CMD, tempCmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_TEMP_BIT);
}

void TempControlWidget::onFanChToggled(uint16_t ch, bool checked)
{
    uint16_t tempCmd = 0;
    if (checked) {
        tempCmd = TEMP_CMD_FAN_ENABLE;
    } else {
        tempCmd = TEMP_CMD_FAN_DISABLE;
    }
    uint16_t chSet = (uint16_t)0x01 << ch;
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_TEMP_CMD, tempCmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_FAN_CH, chSet);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_TEMP_BIT);
}

void TempControlWidget::onFanSpeedChanged(uint16_t ch, int value)
{
    uint16_t tempCmd = TEMP_CMD_FAN_SET_SPEED;
    uint16_t speed = value * 65535 / 100;
    uint16_t chSet = (uint16_t)(0x01) << ch;

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_TEMP_CMD, tempCmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_FAN_CH, chSet);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_FAN_SPEED, speed);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_TEMP_BIT);
}

void TempControlWidget::onFanControlToggled(bool checked)
{
    uint16_t tempCmd = TEMP_CMD_FAN_SET;
    uint16_t fanChSet = 0;
    QVector<uint16_t> fanSpeed(4, 0);
    for (int i = 0; i < FAN_NUM; i++) {
        if (chkFans[i]->isChecked()) {
            fanChSet |= (uint16_t)0x01 << i;
        }
        fanSpeed[i] = spinFanSpeed[i]->value() * 65535 / 100;
    }

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_TEMP_CMD, tempCmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_FAN_CH, fanChSet);
    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_FAN_1_SPEED, fanSpeed);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_TEMP_BIT);
}

