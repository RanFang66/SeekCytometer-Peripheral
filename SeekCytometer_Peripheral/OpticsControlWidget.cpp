#include "OpticsControlWidget.h"
#include <QGridLayout>
#include <QGroupBox>
#include "ModbusRegistersTable.h"
#include "ModbusMaster.h"

#define MAX_LASER_INTENSITY 3105
#define MAX_LED_INTENSITY   100


OpticsControlWidget::OpticsControlWidget(const QString &tilte, QWidget *parent)
    : QDockWidget{tilte, parent}
{
    initDockWidget();

    connect(btnLaser_1, &ToggleSwitch::toggled, this, &OpticsControlWidget::onLaser1Toggled);
    connect(btnLaser_2, &ToggleSwitch::toggled, this, &OpticsControlWidget::onLaser2Toggled);
    connect(btnLed, &ToggleSwitch::toggled, this, &OpticsControlWidget::onLedToggled);
    connect(spinLaserIntensity_1, &QSpinBox::valueChanged, this, &OpticsControlWidget::onLaser1IntensityChanged);
    connect(spinLaserIntensity_2, &QSpinBox::valueChanged, this, &OpticsControlWidget::onLaser2IntensityChanged);
    connect(spinLedIntensity, &QSpinBox::valueChanged, this, &OpticsControlWidget::onLedIntensityChanged);


}

void OpticsControlWidget::updateStatus(const QVector<uint16_t> &regs)
{
    if (regs.length() < 6) {
         qDebug() << "Wrong status registers number! Need 6, but actually " << regs.length();
        return;
    }
    statusLaser1 = regs[0];
    intensityLaser1 = regs[1];
    statusLaser2 = regs[2];
    intensityLaser2 = regs[3];
    statusLed = regs[4];
    intensityLed = regs[5];

    if (statusLaser1 == 0) {
        statusLightLaser1->setState(StatusLight::State::IDLE);
    } else {
        statusLightLaser1->setState(StatusLight::State::RUNNING);
    }

    if (statusLaser2 == 0) {
        statusLightLaser2->setState(StatusLight::State::IDLE);
    } else {
        statusLightLaser2->setState(StatusLight::State::RUNNING);
    }

    if (statusLed == 0) {
        statusLightLed->setState(StatusLight::State::IDLE);
    } else {
        statusLightLed->setState(StatusLight::State::RUNNING);
    }
}

void OpticsControlWidget::onLaser1Toggled(bool checked)
{
    uint16_t intensity = spinLaserIntensity_1->value() * MAX_LASER_INTENSITY / 100;

    QVector<uint16_t> laserCmd(2, 0);

    if (checked) {
        laserCmd[0] = (LASER_1_ID << 8) | LASER_CMD_SWITCH_ON;
    } else {
        laserCmd[0] = (LASER_1_ID << 8) | LASER_CMD_SWITCH_OFF;
    }

    laserCmd[1] = intensity;

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_LASER_CMD, laserCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_LASER_BIT);
}

void OpticsControlWidget::onLaser2Toggled(bool checked)
{
    uint16_t intensity = spinLaserIntensity_2->value() * MAX_LASER_INTENSITY / 100;

    QVector<uint16_t> laserCmd(2, 0);

    if (checked) {
        laserCmd[0] = (LASER_2_ID << 8) | LASER_CMD_SWITCH_ON;
    } else {
        laserCmd[0] = (LASER_2_ID << 8) | LASER_CMD_SWITCH_OFF;
    }

    laserCmd[1] = intensity;

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_LASER_CMD, laserCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_LASER_BIT);
}

void OpticsControlWidget::onLedToggled(bool checked)
{
    uint16_t intensity = spinLedIntensity->value() * MAX_LED_INTENSITY / 100;

    QVector<uint16_t> ledCmd(2, 0);

    if (checked) {
        ledCmd[0] = LED_CMD_SWITCH_ON;
    } else {
        ledCmd[0] = LED_CMD_SWITCH_OFF;
    }

    ledCmd[1] = intensity;

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_LED_CMD, ledCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_LED_BIT);
}

void OpticsControlWidget::onLaser1IntensityChanged()
{
    uint16_t intensity = spinLaserIntensity_1->value() * MAX_LASER_INTENSITY / 100;

    QVector<uint16_t> laserCmd(2, 0);
    laserCmd[0] = (LASER_1_ID << 8) | LASER_CMD_SET_INTENSITY;
    laserCmd[1] = intensity;

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_LASER_CMD, laserCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_LASER_BIT);
}

void OpticsControlWidget::onLaser2IntensityChanged()
{
    uint16_t intensity = spinLaserIntensity_2->value() * MAX_LASER_INTENSITY / 100;

    QVector<uint16_t> laserCmd(2, 0);
    laserCmd[0] = (LASER_2_ID << 8) | LASER_CMD_SET_INTENSITY;
    laserCmd[1] = intensity;

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_LASER_CMD, laserCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_LASER_BIT);
}

void OpticsControlWidget::onLedIntensityChanged()
{
    uint16_t intensity = spinLedIntensity->value() * MAX_LED_INTENSITY / 100;

    QVector<uint16_t> ledCmd(2, 0);
    ledCmd[0] = LED_CMD_SET_INTENSITY;
    ledCmd[1] = intensity;

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_LED_CMD, ledCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_LED_BIT);
}



void OpticsControlWidget::initDockWidget()
{
    // QGroupBox *groupLaser = new QGroupBox("Laser Control", this);
    // QGroupBox *groupLed = new QGroupBox("LED Control", this);

    btnLaser_1 = new ToggleSwitch(this);
    btnLaser_2 = new ToggleSwitch(this);
    btnLed = new ToggleSwitch(this);

    spinLaserIntensity_1 = new QSpinBox(this); 
    spinLaserIntensity_2 = new QSpinBox(this);
    spinLedIntensity = new QSpinBox(this);

    statusLightLaser1 = new StatusLight(this);
    statusLightLaser2 = new StatusLight(this);
    statusLightLed = new StatusLight(this);


    QWidget *mainWidget = new QWidget(this);
    QGridLayout *laserLayout = new QGridLayout();


    auto addLabel = [&](QGridLayout* lay, int row, int col, const QString &text){
        QLabel *hdr = new QLabel(text, this);
        hdr->setAlignment(Qt::AlignCenter);
        hdr->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        hdr->setFixedHeight(20); // 你可以调整到合适值，例如 18~24
        lay->addWidget(hdr, row, col);
    };


    addLabel(laserLayout, 0, 0, tr("Device"));
    addLabel(laserLayout, 0, 1, tr("Switch On/Off"));
    addLabel(laserLayout, 0, 2, tr("Intensity Set"));
    addLabel(laserLayout, 0, 3, tr("Status"));


    addLabel(laserLayout, 1, 0, tr("Laser-638nm"));
    laserLayout->addWidget(btnLaser_1, 1, 1);
    laserLayout->addWidget(spinLaserIntensity_1, 1, 2);
    laserLayout->addWidget(statusLightLaser1, 1, 3);


    addLabel(laserLayout, 2, 0, tr("Laser-448nm"));
    laserLayout->addWidget(btnLaser_2, 2, 1);
    laserLayout->addWidget(spinLaserIntensity_2, 2, 2);
    laserLayout->addWidget(statusLightLaser2, 2, 3);

    addLabel(laserLayout, 3, 0, tr("LED"));
    laserLayout->addWidget(btnLed, 3, 1);
    laserLayout->addWidget(spinLedIntensity, 3, 2);
    laserLayout->addWidget(statusLightLed, 3, 3);

    mainWidget->setLayout(laserLayout);
    setWidget(mainWidget);
}
