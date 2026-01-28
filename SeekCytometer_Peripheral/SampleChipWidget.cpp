#include "SampleChipWidget.h"
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSpacerItem>
#include "ModbusMaster.h"
#include "ModbusRegistersTable.h"
#include <QThread>


#define MOTOR_CMD(id, cmdType) ((id << 8) | (cmdType))

const QStringList coverStatusStr = {
    "Idle", "Opening", "Closing", "Fault",
};

const QStringList sealStatusStr = {
    "Idle", "Pushing", "Releasing", "Fault",
};

const QStringList churnStatusStr = {
    "Idle", "RuningCW", "RuningCCW", "Fault",
};

const QStringList tempStatusStr = {
    "Idle", "Running", "Fault",
};

SampleChipWidget::SampleChipWidget(const QString &tilte, QWidget *parent)
    : QDockWidget{tilte, parent}
{
    initSampleChipWidget();



    connect(btnOpenCover, &QPushButton::clicked, this, &SampleChipWidget::onOpenCoverClicked);
    connect(btnCloseCover, &QPushButton::clicked, this, &SampleChipWidget::onCloseCoverClicked);
    connect(btnPressSample, &QPushButton::clicked, this, &SampleChipWidget::onPressSampleClicked);
    connect(btnReleaseSample, &QPushButton::clicked, this, &SampleChipWidget::onReleaseSampleClicked);
    connect(btnChurnRunCW, &QPushButton::clicked, this, &SampleChipWidget::onChurnCWClicked);
    connect(btnChurnRunCCW, &QPushButton::clicked, this, &SampleChipWidget::onChurnCCWClicked);
    // connect(btnTempControl, &QPushButton::clicked, this, &SampleChipWidget::onTempControlClicked);
    connect(btnChurnStop, &QPushButton::clicked, this, &SampleChipWidget::onChurnStopClicked);
    connect(btnMoveXBackward, &ArrowButton::clicked, this, &SampleChipWidget::onMoveLeftClicked);
    connect(btnMoveXForward, &ArrowButton::clicked, this, &SampleChipWidget::onMoveRightClicked);
    connect(btnMoveYBackward, &ArrowButton::clicked, this, &SampleChipWidget::onMoveBackwardClicked);
    connect(btnMoveYForward, &ArrowButton::clicked, this, &SampleChipWidget::onMoveForwardClicked);
    connect(btnMoveLenDown, &ArrowButton::clicked, this, &SampleChipWidget::onMoveDownClicked);
    connect(btnMoveLenUp, &ArrowButton::clicked, this, &SampleChipWidget::onMoveUpClicked);
    connect(btnXGoZero, &QPushButton::clicked, this, &SampleChipWidget::onXReturnZeroClicked);
    connect(btnYGoZero, &QPushButton::clicked, this, &SampleChipWidget::onYReturnZeroClicked);
    connect(btnZGoZero, &QPushButton::clicked, this, &SampleChipWidget::onZReturnZeroClicked);
    connect(btnXGoPos, &QPushButton::clicked, this, &SampleChipWidget::onXRunToPosClicked);
    connect(btnYGoPos, &QPushButton::clicked, this, &SampleChipWidget::onYRunToPosClicked);
    connect(btnZGoPos, &QPushButton::clicked, this, &SampleChipWidget::onZRunToPosClicked);
}

void SampleChipWidget::updateStatus(const QVector<uint16_t> &regs)
{
    if (regs.length() < 13) {
        qDebug() << "Wrong status registers number! Need 13, but actually " << regs.length();
        return;
    }
    coverStatus = regs[0];
    sealStatus = regs[1];
    churnStatus= regs[2];
    // tempCtrlStatus = regs[3];
    motorXStatus = regs[3];
    motorYStatus = regs[4];
    motorZStatus = regs[5];

    motorXLimit = regs[6] & 0x0003;
    motorYLimit = (regs[6] >> 2) & 0x0003;
    motorZLimit = (regs[6] >> 4) & 0x0003;
    motorXPos = ((uint32_t)regs[7] << 16 | (uint32_t)regs[8]);
    motorYPos = ((uint32_t)regs[9] << 16 | (uint32_t)regs[10]);
    motorZPos = ((uint32_t)regs[11] << 16 | (uint32_t)regs[12]);

    lblCoverStatus->setText(coverStatusStr[coverStatus]);
    lblPressStatus->setText(sealStatusStr[sealStatus]);
    lblChurnStatus->setText(churnStatusStr[churnStatus]);
    // lblTempContorlStatus->setText(tempStatusStr[tempCtrlStatus]);
    lblMotorXStatus->setText(QString::asprintf("X Status: 0x%4x", motorXStatus));
    lblMotorYStatus->setText(QString::asprintf("Y Status: 0x%4x", motorYStatus));
    lblMotorZStatus->setText(QString::asprintf("Len Status: 0x%4x", motorZStatus));
    lblChipPos->setText(QString("X: %1, Y: %2").arg(motorXPos).arg(motorYPos));
    lblLenPos->setText(QString("Len(Z) Pos: %1").arg(motorZPos));
}


void SampleChipWidget::onOpenCoverClicked()
{
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_COVER_CMD, COVER_CMD_OPEN);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_COVER_BIT);
}

void SampleChipWidget::onCloseCoverClicked()
{
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_COVER_CMD, COVER_CMD_CLOSE);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_COVER_BIT);
}

void SampleChipWidget::onPressSampleClicked()
{
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_SEAL_CMD, SEAL_CMD_PUSH);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_SEAL_BIT);
}

void SampleChipWidget::onReleaseSampleClicked()
{
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_SEAL_CMD, SEAL_CMD_RELEASE);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_SEAL_BIT);
}

void SampleChipWidget::onChurnCWClicked()
{
    uint16_t speed = spinChurnSpeed->value() * 160 / 60;

    const QVector<uint16_t> churnCmd = {CHURN_CMD_RUN_CW, speed};
    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_CHURN_CMD, churnCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_CHURN_BIT);
}

void SampleChipWidget::onChurnCCWClicked()
{
    uint16_t speed = spinChurnSpeed->value() * 160 / 60;

    const QVector<uint16_t> churnCmd = {CHURN_CMD_RUN_CCW, speed};
    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_CHURN_CMD, churnCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_CHURN_BIT);
}

void SampleChipWidget::onChurnStopClicked()
{
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CHURN_CMD, CHURN_CMD_STOP);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_CHURN_BIT);
}

// void SampleChipWidget::onTempControlClicked()
// {
//     double val = spinTargetTemp->value();
//     uint16_t tempTarget = val * 10;


//     if (btnTempControl->text() == QString(tr("Temp Control Start"))) {
//         const QVector<uint16_t> tempCmd = {TEMP_CMD_START, tempTarget};
//         ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_TEMP_CMD, tempCmd);
//         ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
//         ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_TEMP_BIT);
//         btnTempControl->setText(QString(tr("Temp Control Stop")));
//     } else {
//         ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_TEMP_CMD, TEMP_CMD_STOP);
//         ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
//         ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_TEMP_BIT);
//         btnTempControl->setText(QString(tr("Temp Control Start")));
//     }
// }

void SampleChipWidget::onMoveLeftClicked()
{
    uint16_t cmd = (MOTOR_X << 8) | MOTOR_CMD_RUN_STEPS;
    int32_t steps = spinXSteps->value() * -1;

    QVector<uint16_t> motorCmd(3, 0);

    motorCmd[0] = cmd;
    motorCmd[1] = (uint16_t)((steps >> 16) & 0x0000FFFF);
    motorCmd[2] = (uint16_t)(steps & 0x0000FFFF);
    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, motorCmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

void SampleChipWidget::onMoveRightClicked()
{
    uint16_t cmd = (MOTOR_X << 8) | MOTOR_CMD_RUN_STEPS;
    int32_t steps = spinXSteps->value();

    QVector<uint16_t> motorCmd(3, 0);

    motorCmd[0] = cmd;
    motorCmd[1] = (uint16_t)((steps >> 16) & 0x0000FFFF);
    motorCmd[2] = (uint16_t)(steps & 0x0000FFFF);

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, motorCmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

void SampleChipWidget::onMoveForwardClicked()
{
    uint16_t cmd = (MOTOR_Y << 8) | MOTOR_CMD_RUN_STEPS;
    int32_t steps = spinYSteps->value() * -1;

    QVector<uint16_t> motorCmd(3, 0);

    motorCmd[0] = cmd;
    motorCmd[1] = (uint16_t)((steps >> 16) & 0x0000FFFF);
    motorCmd[2] = (uint16_t)(steps & 0x0000FFFF);

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, motorCmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

void SampleChipWidget::onMoveBackwardClicked()
{
    uint16_t cmd = (MOTOR_Y << 8) | MOTOR_CMD_RUN_STEPS;
    int32_t steps = spinYSteps->value();

    QVector<uint16_t> motorCmd(3, 0);

    motorCmd[0] = cmd;
    motorCmd[1] = (uint16_t)((steps >> 16) & 0x0000FFFF);
    motorCmd[2] = (uint16_t)(steps & 0x0000FFFF);
    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, motorCmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

void SampleChipWidget::onMoveUpClicked()
{
    uint16_t cmd = (MOTOR_Z << 8) | MOTOR_CMD_RUN_STEPS;
    int32_t steps = spinZSteps->value() * -1;

    QVector<uint16_t> motorCmd(3, 0);

    motorCmd[0] = cmd;
    motorCmd[1] = (uint16_t)((steps >> 16) & 0x0000FFFF);
    motorCmd[2] = (uint16_t)(steps & 0x0000FFFF);

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, motorCmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

void SampleChipWidget::onMoveDownClicked()
{
    uint16_t cmd = (MOTOR_Z << 8) | MOTOR_CMD_RUN_STEPS;
    int32_t steps = spinZSteps->value();

    QVector<uint16_t> motorCmd(3, 0);

    motorCmd[0] = cmd;
    motorCmd[1] = (uint16_t)((steps >> 16) & 0x0000FFFF);
    motorCmd[2] = (uint16_t)(steps & 0x0000FFFF);

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, motorCmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}



void SampleChipWidget::onXReturnZeroClicked()
{
    uint16_t cmd = MOTOR_CMD(MOTOR_X, MOTOR_CMD_RETURN_HOME);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, cmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

void SampleChipWidget::onYReturnZeroClicked()
{
    uint16_t cmd = MOTOR_CMD(MOTOR_Y, MOTOR_CMD_RETURN_HOME);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, cmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

void SampleChipWidget::onZReturnZeroClicked()
{
    uint16_t cmd = MOTOR_CMD(MOTOR_Z, MOTOR_CMD_RETURN_HOME);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, cmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

void SampleChipWidget::onXRunToPosClicked()
{
    uint16_t cmd = MOTOR_CMD(MOTOR_X, MOTOR_CMD_RUN_POS);
    int32_t pos = spinXPos->value();
    QVector<uint16_t> motorCmd(3, 0);

    motorCmd[0] = cmd;
    motorCmd[1] = (uint16_t)((pos >> 16) & 0x0000FFFF);
    motorCmd[2] = (uint16_t)(pos & 0x0000FFFF);

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, motorCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

void SampleChipWidget::onYRunToPosClicked()
{
    uint16_t cmd = MOTOR_CMD(MOTOR_Y, MOTOR_CMD_RUN_POS);
    int32_t pos = spinYPos->value();
    QVector<uint16_t> motorCmd(3, 0);

    motorCmd[0] = cmd;
    motorCmd[1] = (uint16_t)((pos >> 16) & 0x0000FFFF);
    motorCmd[2] = (uint16_t)(pos & 0x0000FFFF);

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, motorCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

void SampleChipWidget::onZRunToPosClicked()
{
    uint16_t cmd = MOTOR_CMD(MOTOR_Z, MOTOR_CMD_RUN_POS);
    int32_t pos = spinZPos->value();
    QVector<uint16_t> motorCmd(3, 0);

    motorCmd[0] = cmd;
    motorCmd[1] = (uint16_t)((pos >> 16) & 0x0000FFFF);
    motorCmd[2] = (uint16_t)(pos & 0x0000FFFF);

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, motorCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}




void SampleChipWidget::initSampleChipWidget()
{
    QGroupBox *groupChipMove = new QGroupBox("Chip Move", this);
    QGroupBox *groupLenMove = new QGroupBox("Len Move", this);
    QGroupBox *groupCover = new QGroupBox("Cover Control", this);
    QGroupBox *groupSeal = new QGroupBox("Seal Control", this);
    QGroupBox *groupChurn = new QGroupBox("Churn Control", this);
    // QGroupBox *groupTempControl = new QGroupBox("Temperature Control", this);
    // QGroupBox *groupFanControl = new QGroupBox("Fan Control", this);

    auto addSpin = [&](QGridLayout *lay, int row, int col, const QString &prompt, QAbstractSpinBox *spinbox) {
        QLabel *lblPrompt = new QLabel(prompt, this);
        QHBoxLayout *layout = new QHBoxLayout();
        layout->addWidget(lblPrompt, 0, Qt::AlignCenter);
        layout->addWidget(spinbox, 0, Qt::AlignCenter);
        lay->addLayout(layout, row, col, Qt::AlignCenter);
    };

    auto addSpinButton = [&](QGridLayout *lay, int row, int col, QPushButton *button, QSpinBox *spinbox) {
        QHBoxLayout *layout = new QHBoxLayout();
        layout->addWidget(button, 0, Qt::AlignCenter);
        layout->addWidget(spinbox, 0, Qt::AlignCenter);
        lay->addLayout(layout, row, col, Qt::AlignCenter);
    };


    btnMoveXBackward = new ArrowButton(ArrowButton::Direction::Left, this);
    btnMoveXForward = new ArrowButton(ArrowButton::Direction::Right, this);
    btnMoveYBackward = new ArrowButton(ArrowButton::Direction::Down, this);
    btnMoveYForward = new ArrowButton(ArrowButton::Direction::Up, this);

    btnXGoZero = new QPushButton("Go X Zero", this);
    btnYGoZero = new QPushButton("Go Y Zero", this);
    btnZGoZero = new QPushButton("Go Z Zero", this);

    btnXGoPos = new QPushButton("Go X Pos", this);
    btnYGoPos = new QPushButton("Go Y Pos", this);
    btnZGoPos = new QPushButton("Go Z Pos", this);

    spinXSteps = new QSpinBox(this);
    spinYSteps = new QSpinBox(this);
    spinZSteps = new QSpinBox(this);

    spinXSteps->setRange(1, 50000);
    spinYSteps->setRange(1, 100000);
    spinZSteps->setRange(1, 10000);
    spinXSteps->setSingleStep(100);
    spinYSteps->setSingleStep(100);
    spinZSteps->setSingleStep(100);
    spinXSteps->setValue(10000);
    spinYSteps->setValue(10000);
    spinZSteps->setValue(5000);

    spinXPos = new QSpinBox(this);
    spinYPos = new QSpinBox(this);
    spinZPos = new QSpinBox(this);
    spinXPos->setRange(-100000, 70000);
    spinXPos->setSingleStep(1000);
    spinXPos->setValue(-24000);
    spinYPos->setRange(-30000, 700000);
    spinYPos->setSingleStep(1000);
    spinYPos->setValue(7885);
    spinZPos->setRange(-10000, 35000);
    spinZPos->setSingleStep(1000);
    spinZPos->setValue(-7600);
    // btnMoveXBackward->setFixedSize(64, 64);
    // btnMoveXForward->setFixedSize(64, 64);
    // btnMoveYBackward->setFixedSize(64, 64);
    // btnMoveYForward->setFixedSize(64, 64);

    btnMoveLenUp = new ArrowButton(ArrowButton::Direction::Up, this);
    btnMoveLenDown = new ArrowButton(ArrowButton::Direction::Down, this);

    btnOpenCover = new QPushButton("Open Cover", this);
    btnCloseCover = new QPushButton("Close Cover", this);

    btnPressSample = new QPushButton("Press Sample", this);
    btnReleaseSample = new QPushButton("Release Sample", this);

    btnChurnRunCW = new QPushButton("Churn CW", this);
    btnChurnRunCCW = new QPushButton("Churn CCW", this);
    btnChurnStop = new QPushButton("Churn Stop", this);
    // btnTempControl = new QPushButton("Temp Control Start", this);

    lblCoverStatus = new QLabel("Cover: Opened", this);
    lblPressStatus = new QLabel("Presser: Released", this);
    lblChurnStatus = new QLabel("Churn: IDLE", this);
    // lblTempContorlStatus = new QLabel("Current: 25 ℃", this);
    lblChipPos = new QLabel("X Pos: 100, \nY Pos: 10000", this);
    lblMotorXStatus = new QLabel("X Status: 0");
    lblMotorYStatus = new QLabel("Y Status: 0");
    lblMotorZStatus = new QLabel("Len Status: 0");
    lblLenPos = new QLabel("Len(Z) Pos: 10000", this);




    spinChurnSpeed = new QSpinBox(this);
    spinChurnSpeed->setMinimum(10);
    spinChurnSpeed->setMaximum(2000);
    spinChurnSpeed->setValue(120);

    // spinTargetTemp = new QDoubleSpinBox(this);
    // spinTargetTemp->setMinimum(0.0);
    // spinTargetTemp->setDecimals(1);
    // spinTargetTemp->setMaximum(30.0);
    // spinTargetTemp->setSingleStep(0.1);
    // spinTargetTemp->setValue(4.0);



    QHBoxLayout *coverLayout = new QHBoxLayout();
    coverLayout->addWidget(btnOpenCover);
    coverLayout->addWidget(btnCloseCover);
    coverLayout->addWidget(lblCoverStatus);

    QHBoxLayout *sealLayout = new QHBoxLayout();
    sealLayout->addWidget(btnPressSample);
    sealLayout->addWidget(btnReleaseSample);
    sealLayout->addWidget(lblPressStatus);


    QHBoxLayout *churnLayout = new QHBoxLayout();
    churnLayout->addWidget(btnChurnRunCW);
    churnLayout->addWidget(btnChurnRunCCW);
    churnLayout->addWidget(btnChurnStop);
    churnLayout->addWidget(spinChurnSpeed);
    churnLayout->addWidget(lblChurnStatus);


    // QGridLayout *tempControlLayout = new QGridLayout();
    // tempControlLayout->addWidget(new QLabel(tr("Target Temp(℃):"), this), 0, 0);
    // tempControlLayout->addWidget(spinTargetTemp, 0, 1);
    // tempControlLayout->addWidget(btnTempControl, 0, 2);
    // tempControlLayout->addWidget(new QLabel(tr("Current Temp(℃):"), this), 1, 0);
    // tempControlLayout->addWidget(lblTempContorlStatus, 1, 1, 1, 2);
    // QGridLayout *fanLayout = new QGridLayout();
    // for (int i = 0; i < FAN_NUM; i++) {
    //     chkFans[i] = new ToggleSwitch(this);
    //     spinFanSpeed[i] = new QSpinBox(this);
    //     spinFanSpeed[i]->setRange(0, 100);
    //     spinFanSpeed[i]->setValue(0);
    //     fanLayout->addWidget(new QLabel(QString("Fan-%1").arg(i+1), this), i, 0, Qt::AlignCenter);
    //     fanLayout->addWidget(chkFans[i], i, 1);
    //     fanLayout->addWidget(spinFanSpeed[i], i, 2);
    // }
    // btnFanEnable = new QPushButton("Switch On", this);
    // btnFanEnable->setCheckable(true);
    // fanLayout->addWidget(btnFanEnable, 4, 0, 1, 2);



    QGridLayout *chipMoveLayout = new QGridLayout();
    chipMoveLayout->addWidget(lblMotorXStatus, 0, 0, Qt::AlignCenter);
    chipMoveLayout->addWidget(lblMotorYStatus, 0, 2, Qt::AlignCenter);
    chipMoveLayout->addWidget(btnXGoZero, 1, 0, Qt::AlignCenter);
    chipMoveLayout->addWidget(btnYGoZero, 1, 2, Qt::AlignCenter);
    addSpin(chipMoveLayout, 2, 0, "X Steps:", spinXSteps);
    addSpin(chipMoveLayout, 2, 2, "Y Steps:", spinYSteps);
    chipMoveLayout->addWidget(btnMoveYForward, 3, 1);
    chipMoveLayout->addWidget(btnMoveXBackward, 4, 0);
    chipMoveLayout->addWidget(lblChipPos, 4, 1, Qt::AlignCenter);
    chipMoveLayout->addWidget(btnMoveXForward, 4, 2);
    chipMoveLayout->addWidget(btnMoveYBackward, 5, 1);
    addSpinButton(chipMoveLayout, 6, 0, btnXGoPos, spinXPos);
    addSpinButton(chipMoveLayout, 6, 2, btnYGoPos, spinYPos);


    QVBoxLayout *lenMoveLayout = new QVBoxLayout();
    lenMoveLayout->addWidget(lblMotorZStatus, 0, Qt::AlignCenter);
    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(new QLabel("Len Steps:", this), 0, Qt::AlignCenter);
    layout->addWidget(spinZSteps, 0, Qt::AlignCenter);
    lenMoveLayout->addWidget(btnZGoZero);
    lenMoveLayout->addLayout(layout);
    lenMoveLayout->addWidget(btnMoveLenUp);
    lenMoveLayout->addWidget(lblLenPos);
    lenMoveLayout->addWidget(btnMoveLenDown);
    QHBoxLayout *layout2 = new QHBoxLayout();
    layout2->addWidget(btnZGoPos);
    layout2->addWidget(spinZPos);
    lenMoveLayout->addLayout(layout2);


    groupChipMove->setLayout(chipMoveLayout);
    groupLenMove->setLayout(lenMoveLayout);
    groupChurn->setLayout(churnLayout);
    groupCover->setLayout(coverLayout);
    groupSeal->setLayout(sealLayout);
    // groupTempControl->setLayout(tempControlLayout);
    // groupFanControl->setLayout(fanLayout);

    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
    QHBoxLayout *moveLayout = new QHBoxLayout();
    moveLayout->addWidget(groupChipMove, 3);
    moveLayout->addWidget(groupLenMove, 1);
    mainLayout->addWidget(groupCover, 1);
    mainLayout->addWidget(groupSeal, 1);
    mainLayout->addWidget(groupChurn, 1);
    // mainLayout->addWidget(groupTempControl, 2);
    // mainLayout->addWidget(groupFanControl, 3);
    mainLayout->addLayout(moveLayout, 6);

    setWidget(mainWidget);
}


